#!/usr/bin/env python3
"""Markdown -> PDF dla dokumentow konsoli (API, instrukcja ucznia, README lekcji).

Sposob: Markdown -> HTML (pakiet `markdown`, tabele, kod) -> PDF przez Microsoft Edge w trybie headless
(jest na kazdym Windows 10/11, obsluguje polskie znaki, CSS i obrazki). Bez pandoca i LaTeX-a.

    python tools/md2pdf.py                # wszystko: docs/API.md, docs/DLA_UCZNIA.md, README kazdej lekcji -> docs/pdf/
    python tools/md2pdf.py docs/API.md    # jeden plik -> docs/pdf/API.pdf
    python tools/md2pdf.py plik.md -o wyjscie.pdf

Wymaga: python -m pip install --user markdown (python = Python dla Windows, nie ten z MSYS2). Sciezki obrazkow w Markdown sa wzgledne do pliku .md - HTML jest zapisywany
obok niego (tymczasowo), wiec obrazki dzialaja.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    import markdown
except ImportError:
    sys.exit("Brak pakietu markdown: python -m pip install --user markdown")

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "docs" / "pdf"

EDGE_CANDIDATES = [
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
]

CSS = """
@page { size: A4; margin: 16mm 16mm 18mm 16mm; }
body { font-family: "Segoe UI", Arial, sans-serif; font-size: 11pt; line-height: 1.45; color: #1e293b; max-width: 100%; }
h1 { font-size: 22pt; color: #0f172a; border-bottom: 2px solid #3b82f6; padding-bottom: 4px; margin-top: 0; }
h2 { font-size: 15pt; color: #1d4ed8; margin-top: 22px; margin-bottom: 6px; page-break-after: avoid; }
h3 { font-size: 12.5pt; color: #0f172a; margin-top: 16px; margin-bottom: 4px; page-break-after: avoid; }
p, li { orphans: 3; widows: 3; }
code { font-family: Consolas, "Courier New", monospace; font-size: 9.5pt; background: #f1f5f9; padding: 1px 4px; border-radius: 3px; }
pre { background: #0f172a; color: #e2e8f0; padding: 10px 12px; border-radius: 6px; font-size: 9pt; line-height: 1.35;
      white-space: pre-wrap; page-break-inside: avoid; }
pre code { background: none; color: inherit; padding: 0; font-size: inherit; }
table { border-collapse: collapse; width: 100%; margin: 8px 0 14px; font-size: 9.5pt; page-break-inside: avoid; }
th, td { border: 1px solid #cbd5e1; padding: 4px 7px; vertical-align: top; text-align: left; }
th { background: #e2e8f0; }
tr:nth-child(even) td { background: #f8fafc; }
img { max-width: 100%; border: 1px solid #cbd5e1; border-radius: 4px; }
blockquote { border-left: 4px solid #94a3b8; margin: 8px 0; padding: 2px 12px; color: #475569; background: #f8fafc; }
ol li, ul li { margin-bottom: 3px; }
hr { border: 0; border-top: 1px solid #cbd5e1; }
.footer { color: #64748b; font-size: 8.5pt; margin-top: 24px; border-top: 1px solid #cbd5e1; padding-top: 4px; }
"""


def find_browser() -> str:
    for p in EDGE_CANDIDATES:
        if Path(p).exists():
            return p
    sys.exit("Nie znaleziono Edge ani Chrome (potrzebny do zapisu PDF)")


def md_to_html(md_path: Path) -> str:
    text = md_path.read_text(encoding="utf-8")
    body = markdown.markdown(text, extensions=["tables", "fenced_code", "sane_lists", "toc"], output_format="html5")
    rel = md_path.relative_to(ROOT).as_posix()
    return f"""<!doctype html><html lang="pl"><head><meta charset="utf-8"><title>{md_path.stem}</title>
<style>{CSS}</style></head><body>{body}
<div class="footer">Konsola ESP32-P4 &middot; {rel} &middot; wygenerowano z Markdown (tools/md2pdf.py)</div></body></html>"""


def convert(md_path: Path, pdf_path: Path, browser: str) -> None:
    html_path = md_path.with_suffix(".tmp.html")   # obok .md, zeby wzgledne obrazki dzialaly
    html_path.write_text(md_to_html(md_path), encoding="utf-8")
    pdf_path.parent.mkdir(parents=True, exist_ok=True)
    # Swiezy profil przy kazdym pliku: Edge trzyma w profilu cache obrazkow i po zmianie PNG wstawial stary.
    profile = Path(tempfile.gettempdir()) / "console_md2pdf_profile"
    shutil.rmtree(profile, ignore_errors=True)
    # Edge zapisuje do pliku tymczasowego, potem podmiana: Edge NIE zglasza bledu, gdy docelowy PDF jest otwarty
    # w innym programie (zostawia stary plik) - tu przy podmianie dostaniemy czytelny wyjatek.
    tmp_pdf = pdf_path.with_suffix(".tmp.pdf")
    tmp_pdf.unlink(missing_ok=True)
    cmd = [
        browser, "--headless=new", "--disable-gpu", "--no-first-run", "--no-default-browser-check",
        f"--user-data-dir={profile}", "--no-pdf-header-footer",
        f"--print-to-pdf={tmp_pdf}", html_path.resolve().as_uri(),
    ]
    try:
        subprocess.run(cmd, check=True, capture_output=True, timeout=120)
    finally:
        html_path.unlink(missing_ok=True)
    if not tmp_pdf.exists():
        sys.exit(f"PDF nie powstal: {pdf_path} (Edge nic nie zapisal)")
    try:
        os.replace(tmp_pdf, pdf_path)
    except PermissionError:
        tmp_pdf.unlink(missing_ok=True)
        sys.exit(f"Nie moge nadpisac {pdf_path.relative_to(ROOT).as_posix()} - zamknij go w przegladarce/czytniku PDF i uruchom ponownie")
    print(f"{md_path.relative_to(ROOT).as_posix()} -> {pdf_path.relative_to(ROOT).as_posix()} ({pdf_path.stat().st_size // 1024} kB)")


def default_jobs() -> list[tuple[Path, Path]]:
    jobs = [
        (ROOT / "docs" / "API.md", OUT_DIR / "API.pdf"),
        (ROOT / "docs" / "DLA_UCZNIA.md", OUT_DIR / "DLA_UCZNIA.pdf"),
    ]
    lekcje = ROOT / "src" / "games" / "lekcje"
    for d in sorted(lekcje.iterdir()):
        readme = d / "README.md"
        if d.is_dir() and readme.exists():
            jobs.append((readme, OUT_DIR / f"lekcja_{d.name}.pdf"))
    return jobs


def main() -> None:
    ap = argparse.ArgumentParser(description="Markdown -> PDF (Edge headless)")
    ap.add_argument("files", nargs="*", help="pliki .md (brak = wszystkie dokumenty konsoli)")
    ap.add_argument("-o", "--output", help="plik wyjsciowy PDF (tylko dla jednego pliku wejsciowego)")
    args = ap.parse_args()

    browser = find_browser()
    if args.files:
        for f in args.files:
            md = Path(f).resolve()
            out = Path(args.output).resolve() if (args.output and len(args.files) == 1) else OUT_DIR / (md.stem + ".pdf")
            convert(md, out, browser)
    else:
        if OUT_DIR.exists():
            for old in OUT_DIR.glob("*.pdf"):
                old.unlink()
        for md, out in default_jobs():
            convert(md, out, browser)


if __name__ == "__main__":
    main()
