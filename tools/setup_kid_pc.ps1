# Instalacja srodowiska do lekcji na komputerze ucznia (Windows 10/11 x64), BEZ PlatformIO i ESP-IDF.
# Tylko emulator: MSYS2 (g++), CMake, Ninja, Git, VS Code + dwa rozszerzenia, kopia repozytorium, LVGL, pierwszy build.
#
# Uruchom PowerShell JAKO ADMINISTRATOR i wpisz:
#   powershell -ExecutionPolicy Bypass -File tools\setup_kid_pc.ps1
# albo (gdy repozytorium jeszcze nie jest sciagniete) sciagnij sam ten plik i uruchom go z dowolnego miejsca:
#   powershell -ExecutionPolicy Bypass -File setup_kid_pc.ps1 -Dir C:\Gry\LakeMarioGame
#
# Parametry:
#   -Dir          gdzie ma byc repozytorium (domyslnie C:\Gry\LakeMarioGame; sciezka BEZ spacji i polskich znakow)
#   -Repo         adres repozytorium git
#   -Branch       galaz ucznia (domyslnie "lekcje")
#   -GitName      imie do commitow (np. "Kuba"), -GitEmail adres (moze byc rodzica)
#   -SkipInstall  nie instaluj programow (juz sa), tylko repo + LVGL + build + VS Code
#   -LvglZip      zip LVGL z pendrive'a, gdy nie ma internetu (patrz tools/fetch_lvgl.ps1)
# Skrypt mozna uruchamiac wiele razy - pomija to, co juz zrobione. Log: %USERPROFILE%\lake_setup.log
param(
    [string]$Dir      = "C:\Gry\LakeMarioGame",
    [string]$Repo     = "https://github.com/grekot/ESP32_p4_game_console.git",
    [string]$Branch   = "lekcje",
    [string]$GitName  = "",
    [string]$GitEmail = "",
    [string]$LvglZip  = "",
    [switch]$SkipInstall
)

$ErrorActionPreference = "Stop"
Start-Transcript -Path (Join-Path $env:USERPROFILE "lake_setup.log") -Append | Out-Null

function Step([string]$msg) { Write-Host ""; Write-Host "==> $msg" -ForegroundColor Cyan }
function Ok([string]$msg)   { Write-Host "    OK: $msg" -ForegroundColor Green }
function Fail([string]$msg) { Write-Host "    BLAD: $msg" -ForegroundColor Red; Stop-Transcript | Out-Null; exit 1 }

function Refresh-Path {
    $m = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $u = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = "$m;$u"
}

function Has-Command([string]$name) {
    return $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

# ------------------------------------------------------------------ 0. warunki wstepne
Step "Sprawdzam warunki"
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { Fail "uruchom PowerShell jako administrator (prawy przycisk -> Uruchom jako administrator)" }
if ($Dir -match "[ \u0080-\uFFFF]") { Fail "sciezka '$Dir' ma spacje albo polskie znaki - kompilator sobie z tym nie radzi. Podaj np. -Dir C:\Gry\LakeMarioGame" }
if (-not [Environment]::Is64BitOperatingSystem) { Fail "potrzebny Windows 64-bitowy" }
Ok "administrator, sciezka $Dir"

# ------------------------------------------------------------------ 1. programy przez winget
if (-not $SkipInstall) {
    Step "Instaluje programy (winget)"
    if (-not (Has-Command winget)) {
        Write-Host "    Brak wingeta. Zainstaluj recznie i uruchom skrypt ponownie z -SkipInstall:" -ForegroundColor Yellow
        Write-Host "      Git:     https://git-scm.com/download/win"
        Write-Host "      CMake:   https://cmake.org/download/  (MSI, zaznacz 'Add CMake to PATH')"
        Write-Host "      Ninja:   https://github.com/ninja-build/ninja/releases  (ninja.exe do C:\Prg\ninja i do PATH)"
        Write-Host "      VS Code: https://code.visualstudio.com/"
        Write-Host "      MSYS2:   https://www.msys2.org/  (domyslna sciezka C:\msys64), potem w MSYS2:"
        Write-Host "               pacman -Syuu ; pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb"
        Fail "brak wingeta"
    }
    $pkgs = @(
        @{ id = "Git.Git";                      check = { Has-Command git } },
        @{ id = "Kitware.CMake";                check = { Has-Command cmake }; extra = @("--override", "ADD_CMAKE_TO_PATH=System /passive") },
        @{ id = "Ninja-build.Ninja";            check = { Has-Command ninja } },
        @{ id = "Microsoft.VisualStudioCode";   check = { Has-Command code } },
        @{ id = "MSYS2.MSYS2";                  check = { Test-Path "C:\msys64\usr\bin\bash.exe" } }
    )
    foreach ($p in $pkgs) {
        Refresh-Path
        if (& $p.check) { Ok "$($p.id) juz jest"; continue }
        Write-Host "    instaluje $($p.id)..."
        $args = @("install", "-e", "--id", $p.id, "--accept-source-agreements", "--accept-package-agreements", "--silent")
        if ($p.extra) { $args += $p.extra }
        & winget @args
        Refresh-Path
        if (-not (& $p.check)) { Write-Host "    UWAGA: po instalacji $($p.id) nadal nie widac polecenia - byc moze trzeba otworzyc nowe okno" -ForegroundColor Yellow }
    }

    Step "MSYS2: kompilator g++ i debugger gdb"
    $bash = "C:\msys64\usr\bin\bash.exe"
    if (-not (Test-Path "C:\msys64\mingw64\bin\g++.exe")) {
        # Pierwsza aktualizacja rdzenia MSYS2 potrafi zakonczyc powloke ("close terminal") - dlatego dwa przebiegi.
        & $bash -lc "pacman -Syuu --noconfirm" 2>$null
        & $bash -lc "pacman -Syuu --noconfirm" 2>$null
        & $bash -lc "pacman -S --needed --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb"
    }
    if (-not (Test-Path "C:\msys64\mingw64\bin\g++.exe")) { Fail "nie ma C:\msys64\mingw64\bin\g++.exe - uruchom MSYS2 i wpisz: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb" }
    Ok (& "C:\msys64\mingw64\bin\g++.exe" --version | Select-Object -First 1)
}

Refresh-Path
Step "Sprawdzam narzedzia"
foreach ($t in @("git", "cmake", "ninja", "code")) {
    if (-not (Has-Command $t)) { Fail "brak polecenia '$t' w PATH. Zamknij i otworz PowerShell jako administrator, uruchom skrypt ponownie (z -SkipInstall, jesli programy sa zainstalowane)." }
    Ok ("{0}: {1}" -f $t, ((& $t --version 2>$null | Select-Object -First 1) -replace "`r|`n", ""))
}

# ------------------------------------------------------------------ 2. VS Code: rozszerzenia i ustawienia
Step "VS Code: rozszerzenia C/C++ i CMake Tools (bez PlatformIO)"
foreach ($ext in @("ms-vscode.cpptools", "ms-vscode.cmake-tools")) {
    & code --install-extension $ext --force 2>$null | Out-Null
    Ok $ext
}

Step "VS Code: ustawienia uzytkownika"
$codeUser = Join-Path $env:APPDATA "Code\User"
New-Item -ItemType Directory -Path $codeUser -Force | Out-Null
$settingsPath = Join-Path $codeUser "settings.json"
$settings = New-Object PSObject
if (Test-Path $settingsPath) {
    $raw = Get-Content $settingsPath -Raw
    if ($raw.Trim() -ne "") { try { $settings = $raw | ConvertFrom-Json } catch { Write-Host "    settings.json nieczytelny - zaczynam od nowego" -ForegroundColor Yellow } }
}
$want = @{
    "C_Cpp.default.configurationProvider" = "ms-vscode.cmake-tools"   # IntelliSense z CMake (u rodzica robi to PlatformIO)
    "C_Cpp.default.compilerPath"          = "C:/msys64/mingw64/bin/g++.exe"
    "C_Cpp.default.intelliSenseMode"      = "windows-gcc-x64"
    "extensions.ignoreRecommendations"    = $true          # bez dymka "zainstaluj PlatformIO"
    "cmake.showOptionsMovedNotification"  = $false
    "workbench.startupEditor"             = "none"
    "files.autoSave"                      = "afterDelay"   # dziecko zapomina Ctrl+S
    "editor.formatOnType"                 = $false
    "git.enableSmartCommit"               = $true
    "git.confirmSync"                     = $false
}
foreach ($k in $want.Keys) { $settings | Add-Member -MemberType NoteProperty -Name $k -Value $want[$k] -Force }
[IO.File]::WriteAllText($settingsPath, ($settings | ConvertTo-Json -Depth 10), (New-Object Text.UTF8Encoding $false))
Ok $settingsPath

$keysPath = Join-Path $codeUser "keybindings.json"
$keys = @()
if (Test-Path $keysPath) {
    $raw = Get-Content $keysPath -Raw
    if ($raw.Trim() -ne "") { try { $keys = @($raw | ConvertFrom-Json) } catch { $keys = @() } }
}
$hasF6 = $false
foreach ($k in $keys) { if ($k.key -eq "f6") { $hasF6 = $true } }
if (-not $hasF6) {
    $keys += [PSCustomObject]@{ key = "f6"; command = "workbench.action.tasks.runTask"; args = "LEKCJA: Uruchom gre z otwartego pliku" }
    [IO.File]::WriteAllText($keysPath, (ConvertTo-Json @($keys) -Depth 5), (New-Object Text.UTF8Encoding $false))
}
Ok "F6 = zbuduj i uruchom gre z otwartego pliku"

# ------------------------------------------------------------------ 3. repozytorium
Step "Repozytorium"
if (-not (Test-Path (Join-Path $Dir ".git"))) {
    New-Item -ItemType Directory -Path (Split-Path $Dir -Parent) -Force | Out-Null
    & git clone --branch $Branch $Repo $Dir
    if (-not (Test-Path (Join-Path $Dir ".git"))) {
        Write-Host "    galaz '$Branch' moze nie istniec - probuje domyslnej" -ForegroundColor Yellow
        & git clone $Repo $Dir
    }
    if (-not (Test-Path (Join-Path $Dir ".git"))) { Fail "klonowanie nie powiodlo sie" }
}
Push-Location $Dir
if ($GitName -ne "")  { & git config user.name $GitName }
if ($GitEmail -ne "") { & git config user.email $GitEmail }
& git config core.autocrlf false
Ok ("galaz: " + (& git rev-parse --abbrev-ref HEAD))

# ------------------------------------------------------------------ 4. LVGL
Step "LVGL (biblioteka menu konsoli)"
if (Test-Path (Join-Path $Dir "managed_components\lvgl__lvgl\lvgl.h")) {
    Ok "jest kopia z PlatformIO (managed_components)"
} elseif (Test-Path (Join-Path $Dir "third_party\lvgl\lvgl.h")) {
    Ok "jest third_party\lvgl"
} else {
    $fetchArgs = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $Dir "tools\fetch_lvgl.ps1"))
    if ($LvglZip -ne "") { $fetchArgs += @("-Zip", $LvglZip) }
    & powershell @fetchArgs
    if (-not (Test-Path (Join-Path $Dir "third_party\lvgl\lvgl.h"))) { Fail "LVGL nie zostal pobrany. Bez internetu: skopiuj zip i podaj -LvglZip sciezka" }
    Ok "third_party\lvgl"
}

# ------------------------------------------------------------------ 5. pierwszy build emulatora
Step "Pierwszy build emulatora (kilka minut)"
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
& cmake -S sim --preset mingw
if ($LASTEXITCODE -ne 0) { Fail "konfiguracja CMake nie powiodla sie (patrz wyzej)" }
& cmake --build sim/build
if ($LASTEXITCODE -ne 0) { Fail "kompilacja emulatora nie powiodla sie (patrz wyzej)" }
$exe = Join-Path $Dir "sim\build\console_sim.exe"
& $exe --list
if ($LASTEXITCODE -ne 0) { Fail "emulator nie startuje" }
Ok "console_sim.exe dziala"
Pop-Location

# ------------------------------------------------------------------ 6. skrot na pulpicie
Step "Skrot na pulpicie"
$codeCmd = (Get-Command code).Source                    # ...\Microsoft VS Code\bin\code.cmd
$codeExe = Join-Path (Split-Path (Split-Path $codeCmd -Parent) -Parent) "Code.exe"
$desktop = [Environment]::GetFolderPath("Desktop")
$lnk = (New-Object -ComObject WScript.Shell).CreateShortcut((Join-Path $desktop "Lekcje C++.lnk"))
$lnk.TargetPath = $codeExe
$lnk.Arguments  = "`"$Dir`""
$lnk.WorkingDirectory = $Dir
$lnk.IconLocation = $codeExe
$lnk.Save()
Ok "Lekcje C++ -> VS Code z folderem $Dir"

Write-Host ""
Write-Host "GOTOWE." -ForegroundColor Green
Write-Host "  1. Kliknij skrot 'Lekcje C++' na pulpicie."
Write-Host "  2. Otworz src/games/lekcje/01_wizytowka/gra.cpp."
Write-Host "  3. Wcisnij F6 (albo Ctrl+Shift+B) - otworzy sie okno z gra."
Write-Host "  Jesli VS Code zapyta 'Use CMake Tools as IntelliSense provider?' - odpowiedz Yes."
Write-Host "  Instrukcja dla ucznia: docs/DLA_UCZNIA.md"
Stop-Transcript | Out-Null
