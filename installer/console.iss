; Instalator Windows konsoli (emulator + gry + lekcje). Inno Setup 6.
;
; Budowa lokalna:  powershell -File tools/build_installer.ps1 -Version 1.0.0
; Wydanie:         git tag v1.0.0 && git push origin v1.0.0  -> .github/workflows/release.yml buduje i publikuje
;
; Instalacja per uzytkownik (%LOCALAPPDATA%\Programs\KotarbaConsole), bez praw administratora - dzieki temu
; program startowy (KotarbaConsole.exe) moze sam zainstalowac aktualizacje pobrana z GitHuba bez okna UAC.
; Zapisy (rekordy, ustawienia: save_*.bin) leza obok exe i przetrwaja aktualizacje.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\sim\build-release"
#endif
#define AppName "Kotarba Game Console"
#define AppExe  "KotarbaConsole.exe"

[Setup]
; AppId nie moze sie zmienic - po nim instalator rozpoznaje poprzednia wersje przy aktualizacji
AppId={{6F3B1C52-8E4A-4D27-9B1F-2A7C5E0D4B91}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Grzegorz Kotarba
AppPublisherURL=https://github.com/grekot/ESP32_p4_game_console
AppUpdatesURL=https://github.com/grekot/ESP32_p4_game_console/releases
DefaultDirName={autopf}\KotarbaConsole
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableDirPage=yes
DisableReadyPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=KotarbaConsole-{#AppVersion}-setup
SetupIconFile=..\sim\app.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[InstallDelete]
; stare obrazki z poprzedniej wersji (zmienione nazwy plikow nie moga zostac na dysku)
Type: filesandordirs; Name: "{app}\assets"

[Files]
Source: "{#BuildDir}\{#AppExe}";      DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\console_sim.exe"; DestDir: "{app}"; Flags: ignoreversion
; mapowanie klawiszy mozna zmienic recznie - aktualizacja go nie nadpisuje
Source: "..\sim\keymap.cfg";          DestDir: "{app}"; Flags: onlyifdoesntexist
Source: "STEROWANIE.txt";             DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\*";                DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}";            Filename: "{app}\{#AppExe}"
Name: "{autoprograms}\{#AppName} - sterowanie"; Filename: "{app}\STEROWANIE.txt"
Name: "{autodesktop}\{#AppName}";             Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
; zwykla instalacja: pole "Uruchom" na ostatniej stronie
Filename: "{app}\{#AppExe}"; Parameters: "--no-update"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
; aktualizacja z programu startowego (/SILENT /RELAUNCH): uruchom konsole od razu
Filename: "{app}\{#AppExe}"; Parameters: "--no-update"; Flags: nowait; Check: IsRelaunch

[UninstallDelete]
Type: files; Name: "{app}\save_*.bin"
Type: dirifempty; Name: "{app}"

[Code]
function IsRelaunch: Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
    if CompareText(ParamStr(I), '/RELAUNCH') = 0 then
      Result := True;
end;
