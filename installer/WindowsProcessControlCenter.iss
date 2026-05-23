#define MyAppName "Windows Process Control Center"
#define MyAppExeName "WindowsProcessControlCenter.exe"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Windows Process Control Center"
#define MyAppId "{{6FDC4703-94B6-4E3D-98B1-B22588940D1E}"
#define PortableDir "..\dist\WindowsProcessControlCenter-0.1.0-portable"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/Zayoooh1/WindowsProcessControlCenter
AppSupportURL=https://github.com/Zayoooh1/WindowsProcessControlCenter
AppUpdatesURL=https://github.com/Zayoooh1/WindowsProcessControlCenter
DefaultDirName={localappdata}\Programs\WindowsProcessControlCenter
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=no
OutputDir=..\dist\installer
OutputBaseFilename=WindowsProcessControlCenter-0.1.0-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked
Name: "startmenu"; Description: "Create Start Menu folder"; GroupDescription: "Shortcuts:"; Flags: checkedonce
Name: "startup"; Description: "Start with Windows"; GroupDescription: "Startup:"; Flags: unchecked

[Files]
Source: "{#PortableDir}\WindowsProcessControlCenter.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PortableDir}\web\*"; DestDir: "{app}\web"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PortableDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PortableDir}\RELEASE_NOTES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PortableDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startmenu
Name: "{group}\Release Notes"; Filename: "{app}\RELEASE_NOTES.md"; Tasks: startmenu
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"; Tasks: startmenu

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "WindowsProcessControlCenter"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: startup

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\WebView2UserData"

[Code]
function IsWebView2RuntimePresent(): Boolean;
var
  Version: String;
begin
  Result :=
    RegQueryStringValue(HKLM32, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) or
    RegQueryStringValue(HKLM64, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) or
    RegQueryStringValue(HKCU, 'Software\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version);
end;

function InitializeSetup(): Boolean;
begin
  Result := True;

  if not IsWebView2RuntimePresent() then
  begin
    MsgBox(
      'Windows Process Control Center requires Microsoft Edge WebView2 Runtime.' + #13#10 + #13#10 +
      'Windows 11 usually already includes it. If the application window opens but the UI does not load, install Microsoft Edge WebView2 Evergreen Runtime from Microsoft.' + #13#10 + #13#10 +
      'Setup will continue because runtime detection can be incomplete.',
      mbInformation,
      MB_OK);
  end;
end;
