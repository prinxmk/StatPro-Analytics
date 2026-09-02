#define MyAppName "StatPro Analytics"
#define MyAppVersion "0.3.0"
#define MyAppPublisher "StatPro"
#define MyAppExeName "StatProAnalytics.exe"

[Setup]
AppId={{A7EAB0F1-3C0C-4F9B-8E38-STATPROANALYTICS}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\StatPro Analytics
DefaultGroupName=StatPro Analytics
OutputDir=..\Output
OutputBaseFilename=StatPro-Analytics-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Files]
Source: "..\build\Release\StatProAnalytics.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\StatPro Analytics"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\StatPro Analytics"; Filename: "{app}\{#MyAppExeName}"

[Registry]
Root: HKCU; Subkey: "Software\Classes\.stpro"; ValueType: string; ValueName: ""; ValueData: "StatProAnalytics.Project"; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\StatProAnalytics.Project"; ValueType: string; ValueName: ""; ValueData: "StatPro Analytics Project"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\StatProAnalytics.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
