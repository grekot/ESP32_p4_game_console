# Testy regresji emulatora i testy zadan z lekcji.
#
#   powershell -File tools/testy.ps1               # wszystko: Mario + kazda lekcja z plikiem testy.txt
#   powershell -File tools/testy.ps1 mario         # tylko scenariusze z tests/scenarios.txt
#   powershell -File tools/testy.ps1 src/games/lekcje/02_pilka   # testy jednej lekcji (testy.txt)
#   powershell -File tools/testy.ps1 mario -Update # nagraj wzorce na nowo (po SWIADOMEJ zmianie fizyki/wygladu)
#
# tests/scenarios.txt:  rodzaj | nazwa | argumenty     (trace = linie ^TRACE, shot = zrzut BMP; porownanie dokladne)
# <lekcja>/testy.txt:   argumenty | regex              (regex musi wystapic w wyjsciu; --game dokleja skrypt)
# Kod wyjscia: 0 = wszystko OK, 1 = sa bledy.
param(
    [string]$Target = "all",
    [switch]$Update
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Exe  = Join-Path $Root "sim\build\console_sim.exe"
if (-not (Test-Path $Exe)) {
    Write-Host "BLAD: brak $Exe - najpierw zbuduj emulator (cmake --build sim/build)" -ForegroundColor Red
    exit 1
}

$script:Ok = 0
$script:Bad = 0

function Split-Args([string]$s) {
    return @($s.Trim() -split "\s+" | Where-Object { $_ -ne "" })
}

function Run-Sim([string[]]$a) {
    # Tylko stdout (slad TRACE i logi I); stderr (logi W/E) pomijamy, zeby nie zaciemniac porownan.
    $out = & $Exe @a 2>$null
    $code = $LASTEXITCODE
    if ($null -eq $out) { $out = @() }
    return @{ Lines = @($out); Code = $code }
}

function Report([string]$name, [bool]$ok, [string]$detail) {
    if ($ok) {
        $script:Ok++
        Write-Host ("  OK    {0}" -f $name) -ForegroundColor Green
    } else {
        $script:Bad++
        Write-Host ("  BLAD  {0}" -f $name) -ForegroundColor Red
        if ($detail) { Write-Host ($detail -replace "(?m)^", "        ") }
    }
}

function Test-Mario {
    $file = Join-Path $Root "tests\scenarios.txt"
    $expDir = Join-Path $Root "tests\expected"
    if (-not (Test-Path $expDir)) { New-Item -ItemType Directory $expDir | Out-Null }
    Write-Host "Scenariusze: tests/scenarios.txt"
    foreach ($line in Get-Content $file) {
        $t = $line.Trim()
        if ($t -eq "" -or $t.StartsWith("#")) { continue }
        $parts = $t -split "\|"
        if ($parts.Count -lt 3) { Report $t $false "zla linia (oczekiwano: rodzaj | nazwa | argumenty)"; continue }
        $kind = $parts[0].Trim(); $name = $parts[1].Trim(); $args = Split-Args $parts[2]

        if ($kind -eq "trace") {
            $r = Run-Sim $args
            $got = @($r.Lines | Where-Object { $_ -like "TRACE*" })
            $expFile = Join-Path $expDir "$name.trace"
            if ($Update) {
                [IO.File]::WriteAllText($expFile, (($got -join "`n") + "`n"))
                Report $name $true ""
                continue
            }
            if (-not (Test-Path $expFile)) { Report $name $false "brak wzorca $expFile (uzyj -Update)"; continue }
            $exp = @([IO.File]::ReadAllText($expFile) -split "`n" | Where-Object { $_ -ne "" })
            if ($r.Code -ne 0) { Report $name $false "kod wyjscia $($r.Code)"; continue }
            $diff = Compare-Object $exp $got -SyncWindow 0
            if ($diff) {
                $d = ($diff | ForEach-Object { if ($_.SideIndicator -eq "<=") { "- " + $_.InputObject } else { "+ " + $_.InputObject } }) -join "`n"
                Report $name $false $d
            } else { Report $name $true "" }
        }
        elseif ($kind -eq "shot") {
            $expFile = Join-Path $expDir "$name.bmp"
            $tmp = Join-Path $env:TEMP "console_test_$name.bmp"
            $r = Run-Sim ($args + @("--shot", $tmp))
            if ($r.Code -ne 0 -or -not (Test-Path $tmp)) { Report $name $false "emulator nie zapisal zrzutu (kod $($r.Code))"; continue }
            if ($Update) { Copy-Item $tmp $expFile -Force; Report $name $true ""; continue }
            if (-not (Test-Path $expFile)) { Report $name $false "brak wzorca $expFile (uzyj -Update)"; continue }
            $h1 = (Get-FileHash $tmp -Algorithm SHA256).Hash
            $h2 = (Get-FileHash $expFile -Algorithm SHA256).Hash
            if ($h1 -eq $h2) { Report $name $true "" }
            else { Report $name $false "zrzut rozni sie od wzorca; nowy: $tmp" }
        }
        else { Report $name $false "nieznany rodzaj '$kind'" }
    }
}

function Test-Lekcja([string]$dir) {
    $full = Join-Path $Root $dir
    $file = Join-Path $full "testy.txt"
    if (-not (Test-Path $file)) { Write-Host "  (brak testy.txt w $dir)"; return }
    $id = Split-Path $full -Leaf
    Write-Host "Lekcja: $dir"
    $n = 0
    foreach ($line in Get-Content $file) {
        $t = $line.Trim()
        if ($t -eq "" -or $t.StartsWith("#")) { continue }
        $n++
        $idx = $t.IndexOf("|")
        if ($idx -lt 0) { Report "test $n" $false "zla linia (oczekiwano: argumenty | regex)"; continue }
        $args = @("--game", $id) + (Split-Args $t.Substring(0, $idx))
        $regex = $t.Substring($idx + 1).Trim()
        $r = Run-Sim $args
        $text = $r.Lines -join "`n"
        if ($r.Code -ne 0) { Report "test $n" $false "kod wyjscia $($r.Code): $($args -join ' ')"; continue }
        if ($text -match $regex) { Report "test $n  /$regex/" $true "" }
        else {
            $tail = (@($r.Lines | Where-Object { $_ -like "TRACE*" } | Select-Object -Last 3) -join "`n")
            Report "test $n  /$regex/" $false ("nie znaleziono w wyjsciu. Ostatnie linie TRACE:`n" + $tail)
        }
    }
}

Push-Location $Root
try {
    if ($Target -eq "all" -or $Target -eq "mario") { Test-Mario }
    if ($Target -eq "all") {
        $lekcje = Join-Path $Root "src\games\lekcje"
        if (Test-Path $lekcje) {
            Get-ChildItem $lekcje -Directory | Sort-Object Name | ForEach-Object {
                Test-Lekcja ("src/games/lekcje/" + $_.Name)
            }
        }
    }
    elseif ($Target -ne "mario") { Test-Lekcja ($Target -replace "\\", "/") }
} finally { Pop-Location }

Write-Host ""
$color = "Green"
if ($script:Bad -gt 0) { $color = "Red" }
Write-Host ("Wynik: {0} OK, {1} BLAD" -f $script:Ok, $script:Bad) -ForegroundColor $color
if ($script:Bad -gt 0) { exit 1 }
exit 0
