# Simple Tunnel SSH

Simple Tunnel SSH is a lightweight Windows 11 tray application for managing SSH local port forwarding groups with the native `ssh.exe` client.

## Download

- Installer: available in the GitHub release assets for version `0.0.2` as `SimpleTunnelSSH-Installer.exe`
- Portable executable: available in the GitHub release assets for version `0.0.2` as `SimpleTunnelSSH-Executable.exe`

## Features

- Simple WinForms interface focused on low overhead.
- One active SSH connection at a time, with multiple active groups inside that connection.
- Per-group and per-port activation controls with status indicators.
- Conflict detection for local ports across selected groups.
- Native OpenSSH process management with keep-alive, automatic reconnection, and interactive authentication prompts when SSH requests a password or passphrase.
- Built-in runtime log viewer for recent application and SSH events.
- Persistent JSON configuration in `%LocalAppData%\SimpleTunnelSSH\config.json`.
- Import and export for full configuration backup.
- Tray icon controls for opening the window, toggling groups and exiting the app.

## Development prerequisites

- .NET 8 SDK
- Windows OpenSSH client (`C:\Windows\System32\OpenSSH\ssh.exe`)

## Configuration storage

Runtime configuration is stored outside the installation directory at `%LocalAppData%\SimpleTunnelSSH\config.json`.

## License

This project is released under the MIT license. You can use, modify and redistribute it, as long as the copyright and license notice are preserved. See `LICENSE`.

## Run locally

```powershell
& "$env:LOCALAPPDATA\Microsoft\dotnet\dotnet.exe" build SimpleTunnelSSH.sln -c Debug
& "$env:LOCALAPPDATA\Microsoft\dotnet\dotnet.exe" run --project .\src\SimpleTunnelSSH.App\SimpleTunnelSSH.App.csproj -c Debug
```

## Run tests

```powershell
& "$env:LOCALAPPDATA\Microsoft\dotnet\dotnet.exe" test .\tests\SimpleTunnelSSH.Core.Tests\SimpleTunnelSSH.Core.Tests.csproj -c Debug
```

## Publish the final EXE

```powershell
& "$env:LOCALAPPDATA\Microsoft\dotnet\dotnet.exe" publish .\src\SimpleTunnelSSH.App\SimpleTunnelSSH.App.csproj -c Release -p:PublishProfile=WinX64SingleFile
```

The self-contained single-file output is generated in `artifacts\publish\win-x64`.

## Build the installer

```powershell
.\scripts\Publish-Installer.ps1
```

The installer output is generated in `artifacts\installer`.