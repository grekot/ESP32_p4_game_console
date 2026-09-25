# Instalator Windows konsoli: build Release emulatora + programu startowego, potem Inno Setup.
#
#   powershell -File tools/build_installer.ps1                  # wersja 0.0.0-dev (bez sprawdzania aktualizacji)
#   powershell -File tools/build_installer.ps1 -Version 1.0.0   # wynik: dist/KotarbaConsole-1.0.0-setup.exe
#
# Wymaga: MSYS2 MinGW (C:\msys64\mingw64), CMake, Ninja, Inno Setup 6 (ISCC.exe).
# Build idzie do sim/build-release (osobno od sim/build, ktorego uzywaja testy).
# To samo robi CI: .github/workflows/release.yml (po wypchnieciu taga v1.0.0).
param(
    [string]$Version = "dev"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root "sim\build-release"

$env:PATH = "C:\msys64\mingw64\bin;C:\Prg\ninja-win;C:\Program Files\CMake\bin;" + $env:PATH

& cmake -S (Join-Path $root "sim") -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ "-DCONSOLE_VERSION=$Version"
if ($LASTEXITCODE -ne 0) { throw "cmake: konfiguracja nie powiodla sie" }
& cmake --build $build
if ($LASTEXITCODE -ne 0) { throw "cmake: budowa nie powiodla sie (dzialajacy console_sim.exe z build-release?)" }

$iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
          "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw "Brak Inno Setup 6 (ISCC.exe). Instalacja: https://jrsoftware.org/isdl.php" }

$innoVersion = if ($Version -eq "dev") { "0.0.0-dev" } else { $Version }
& $iscc "/DAppVersion=$innoVersion" "/DBuildDir=$build" (Join-Path $root "installer\console.iss")
if ($LASTEXITCODE -ne 0) { throw "ISCC: budowa instalatora nie powiodla sie" }
Get-ChildItem (Join-Path $root "dist\KotarbaConsole-$innoVersion-setup.exe") | Select-Object Name, Length
