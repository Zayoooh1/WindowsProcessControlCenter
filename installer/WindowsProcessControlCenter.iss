#define MyAppName "Windows Process Control Center"
#define MyAppVersion "0.1.10"
#define MyAppPublisher "Windows Process Control Center"
#define MyAppExeName "WindowsProcessControlCenter.exe"
#define MyAppId "{{6FDC4703-94B6-4E3D-98B1-B22588940D1E}"
#define MyAppIcon "..\assets\icon.ico"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}

DefaultDirName={autopf}\Windows Process Control Center
DefaultGroupName=Windows Process Control Center

DisableProgramGroupPage=yes

OutputDir=..\dist
OutputBaseFilename=WindowsProcessControlCenter-v0.1.10-Setup

Compression=lzma2
SolidCompression=yes

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

SetupIconFile={#MyAppIcon}

PrivilegesRequired=admin
WizardStyle=modern

UninstallDisplayName=Windows Process Control Center
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "..\build-release-verify\Release\WindowsProcessControlCenter.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build-release-verify\Release\web\*"; DestDir: "{app}\web"; Flags: ignoreversion recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Icons]
Name: "{group}\Windows Process Control Center"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\Windows Process Control Center"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch Windows Process Control Center"; Flags: nowait postinstall skipifsilent

[Code]
function IsWebView2RuntimePresent(): Boolean;
var
  Version: String;
begin
  Result :=
    RegQueryStringValue(
      HKLM32,
      'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}',
      'pv',
      Version
    ) or
    RegQueryStringValue(
      HKLM64,
      'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}',
      'pv',
      Version
    ) or
    RegQueryStringValue(
      HKCU,
      'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}',
      'pv',
      Version
    );
end;

function InitializeSetup(): Boolean;
begin
  Result := True;

  if not IsWebView2RuntimePresent() then
    MsgBox(
      'Windows Process Control Center requires Microsoft Edge WebView2 Runtime.' + #13#10 + #13#10 +
      'Setup will continue because runtime detection can be incomplete.',
      mbInformation,
      MB_OK
    );
end;

procedure InitializeWizard;
begin
  { Show the destination path, but prevent manual text editing. }
  { The Browse button remains enabled and can still change the path. }
  WizardForm.DirEdit.ReadOnly := True;
end;