# Zamienia zrzuty BMP z emulatora (--shot) na PNG, np. do README i docs/images.
#   powershell -File tools/bmp2png.ps1 zrzut.bmp [drugi.bmp ...]     -> zrzut.png obok
#   powershell -File tools/bmp2png.ps1 zrzut.bmp -Out docs/images/gra.png
param(
    [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$Files,
    [string]$Out = ""
)
Add-Type -AssemblyName System.Drawing
foreach ($f in $Files) {
    $src = (Resolve-Path $f).Path
    if ($Out -ne "" -and $Files.Count -eq 1) { $dst = $Out } else { $dst = [IO.Path]::ChangeExtension($src, ".png") }
    $bmp = [System.Drawing.Bitmap]::FromFile($src)
    try { $bmp.Save($dst, [System.Drawing.Imaging.ImageFormat]::Png) } finally { $bmp.Dispose() }
    Write-Host "$src -> $dst"
}
