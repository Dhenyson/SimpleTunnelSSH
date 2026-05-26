# SimpleTunnelSSH

SimpleTunnelSSH is an open-source native desktop SSH tunnel manager for Windows and Linux.

It is built for low overhead and day-to-day tunnel management around the system OpenSSH client, with reusable connection groups, tray integration, and cross-platform configuration import/export.

## Highlights

- Native desktop implementations for both platforms
- One active SSH connection at a time with per-group and per-port activation
- Automatic reconnection, password/passphrase prompts, and runtime logs
- Local port conflict detection before activation
- Start-with-system support and tray-first workflow
- Import and export of a shared JSON configuration format across Windows and Linux

## Release assets

Each GitHub Release is intended to publish these assets automatically:

- `SimpleTunnelSSH-Win-Executable.exe`
- `SimpleTunnelSSH-Win-Installer.exe`
- `SimpleTunnelSSH-Linux-Installer.deb`

## Platforms

- `SimpleTunnelSSHForWindows/`: WinForms + .NET 8, portable executable and installer
- `SimpleTunnelSSHForLinux/`: C++20 + Qt 6 Widgets, native Linux executable and `.deb` package

## Configuration compatibility

Configuration export/import is designed to work between Windows and Linux.

The main thing that may need adjustment after importing into a different operating system is the SSH private key path, since file system paths differ between Windows and Linux.

## Development

Platform-specific documentation lives here:

- [Windows README](./SimpleTunnelSSHForWindows/README.md)
- [Linux README](./SimpleTunnelSSHForLinux/README.md)

## Open source

- License: MIT, see [LICENSE](./LICENSE)
- Issues and pull requests are welcome