#define AppName "Simple Tunnel SSH"
#define AppVersion "0.0.2"
#define AppPublisher "Dhenyson"
#define AppExeName "SimpleTunnelSSH.exe"

[Setup]
AppId={{9E850170-5B90-4EBF-A4F0-81B496427C02}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\SimpleTunnelSSH
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\artifacts\installer
OutputBaseFilename=SimpleTunnelSSH-Installer
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=..\src\SimpleTunnelSSH.App\Assets\App.ico
UninstallDisplayIcon={app}\{#AppExeName}
UsePreviousAppDir=yes
CloseApplications=yes
RestartApplications=no
AppMutex=SimpleTunnelSSH.Singleton

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"
Name: "startup"; Description: "Start Simple Tunnel SSH when you sign in to Windows"; GroupDescription: "Startup options:"; Flags: unchecked

[Files]
Source: "..\artifacts\publish\win-x64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
Name: "{userstartup}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: startup

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent