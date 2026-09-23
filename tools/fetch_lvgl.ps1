# Pobiera LVGL (ten sam tag, co firmware) do third_party/lvgl - dla komputera BEZ PlatformIO,
# na ktorym nie ma managed_components/lvgl__lvgl. Emulator (sim/CMakeLists.txt) znajdzie ten katalog sam.
#
#   powershell -ExecutionPolicy Bypass -File tools/fetch_lvgl.ps1
#   powershell -ExecutionPolicy Bypass -File tools/fetch_lvgl.ps1 -Zip C:\pendrive\lvgl-9.5.0.zip   # bez internetu
#   powershell -ExecutionPolicy Bypass -File tools/fetch_lvgl.ps1 -SkipHash                       # gdy GitHub zmieni zip
#
# Zostawia tylko to, czego potrzebuje kompilacja (pliki z katalogu glownego, src/, env_support/) -
# bez examples/, demos/, tests/ i docs/ katalog ma ok. 30 MB zamiast 180 MB.
param(
    [string]$Tag  = "v9.5.0",
    [string]$Zip  = "",
    [string]$Sha256 = "ED25A729864E6BE6904CB2E5E0C8566366F4798B694C676B62E10F9B54865697",   # suma zipa v9.5.0 z GitHuba (23.09.2026); pusty = tylko wypisz policzona
    [switch]$SkipHash,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Dest = Join-Path $Root "third_party\lvgl"

if ((Test-Path (Join-Path $Dest "lvgl.h")) -and -not $Force) {
    Write-Host "LVGL juz jest w $Dest (uzyj -Force, zeby pobrac ponownie)"
    exit 0
}

$tmpZip = $Zip
if ($tmpZip -eq "") {
    $url = "https://github.com/lvgl/lvgl/archive/refs/tags/$Tag.zip"
    $tmpZip = Join-Path $env:TEMP "lvgl-$Tag.zip"
    Write-Host "Pobieram $url ..."
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $url -OutFile $tmpZip -UseBasicParsing
}
if (-not (Test-Path $tmpZip)) { Write-Host "BLAD: brak pliku $tmpZip" -ForegroundColor Red; exit 1 }

$hash = (Get-FileHash $tmpZip -Algorithm SHA256).Hash
Write-Host "SHA-256: $hash"
if (-not $SkipHash -and $Sha256 -ne "" -and $hash -ne $Sha256.ToUpper()) {
    Write-Host "BLAD: suma kontrolna sie nie zgadza (oczekiwano $Sha256). Plik uszkodzony albo GitHub zmienil archiwum." -ForegroundColor Red
    Write-Host "Jesli to drugi przypadek: uruchom z -SkipHash i zaktualizuj Sha256 w skrypcie." -ForegroundColor Yellow
    exit 1
}

$extract = Join-Path $env:TEMP "lvgl-extract"
if (Test-Path $extract) { Remove-Item $extract -Recurse -Force }
Write-Host "Rozpakowuje..."
Expand-Archive -Path $tmpZip -DestinationPath $extract -Force
$inner = Get-ChildItem $extract -Directory | Select-Object -First 1   # lvgl-9.5.0/

if (Test-Path $Dest) { Remove-Item $Dest -Recurse -Force }
New-Item -ItemType Directory -Path $Dest -Force | Out-Null

# Tylko to, co jest potrzebne do kompilacji biblioteki: wszystkie pliki z katalogu glownego (CMakeLists.txt,
# naglowki, szablony *.in dla configure_file) oraz src/ i env_support/ - bez examples/, demos/, tests/, docs/, scripts/.
Get-ChildItem $inner.FullName -File | ForEach-Object { Copy-Item $_.FullName -Destination $Dest -Force }
foreach ($sub in @("src", "env_support")) {
    $p = Join-Path $inner.FullName $sub
    if (Test-Path $p) { Copy-Item $p -Destination $Dest -Recurse -Force }
}
Remove-Item $extract -Recurse -Force

$size = [math]::Round((Get-ChildItem $Dest -Recurse -File | Measure-Object Length -Sum).Sum / 1MB, 1)
Write-Host "Gotowe: $Dest ($size MB). Teraz: cmake -S sim --preset mingw" -ForegroundColor Green
