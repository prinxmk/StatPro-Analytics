; StatPro Analytics installer
#define MyAppName "StatPro Analytics"
#define MyAppVersion "0.6.4"
#define MyAppPublisher "StatPro Analytics"
#define MyAppExeName "StatProAnalytics.exe"

[Setup]
AppId={{A4E8B5D5-4F3D-4D8C-9A72-6D0C7A1B2C11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\StatPro Analytics
DefaultGroupName=StatPro Analytics
OutputDir=..\Output
OutputBaseFilename=StatPro-Analytics-Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
UninstallDisplayName={#MyAppName}

[Files]
; Package the complete windeployqt output, including Qt DLLs and plugin folders.
Source: "..\build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\StatPro Analytics"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\StatPro Analytics"; Filename: "{app}\{#MyAppExeName}"

[Registry]
Root: HKCR; Subkey: ".stpro"; ValueType: string; ValueName: ""; ValueData: "StatProAnalyticsProject"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "StatProAnalyticsProject"; ValueType: string; ValueName: ""; ValueData: "StatPro Analytics Project"; Flags: uninsdeletekey
Root: HKCR; Subkey: "StatProAnalyticsProject\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKCR; Subkey: "StatProAnalyticsProject\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch StatPro Analytics"; Flags: nowait postinstall skipifsilent
