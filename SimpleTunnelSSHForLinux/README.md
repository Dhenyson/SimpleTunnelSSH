# Simple Tunnel SSH for Linux

Simple Tunnel SSH for Linux is a native Linux desktop port of the Windows application in this repository.

The Linux version mirrors the Windows project structure as closely as practical while using a native Linux stack focused on low overhead:

- C++20
- Qt 6 Widgets for the desktop UI and tray integration
- OpenSSH client from the host system
- XDG paths for configuration and startup integration

Planned feature parity with the Windows version includes:

- One active SSH connection at a time
- Per-group and per-port activation
- Local port conflict detection
- Automatic reconnection with bounded retries
- Interactive password and passphrase prompts through `SSH_ASKPASS`
- Tray icon controls and start-minimized behavior
- Optional launch at login through XDG autostart
- Import and export of the same JSON configuration schema used by the Windows app

## Configuration path

The Linux port stores runtime configuration in:

```text
~/.config/SimpleTunnelSSH/config.json
```

## Build prerequisites

Ubuntu and Zorin OS packages:

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev-tools
```

The application also requires the OpenSSH client:

```bash
sudo apt install openssh-client
```

You do not need to install those packages manually if you use the provided build scripts on Debian, Ubuntu, or Zorin OS.
The scripts now detect missing build dependencies and install them automatically with `sudo apt-get`.

## Current project layout

The Linux tree intentionally mirrors the Windows tree:

```text
SimpleTunnelSSHForLinux/
  installer/
  scripts/
  src/
    SimpleTunnelSSH.App/
    SimpleTunnelSSH.Core/
  tests/
    SimpleTunnelSSH.Core.Tests/
```

## Status

The core model, configuration store, SSH argument builder, and conflict detection are implemented first so the Linux UI and runtime controller can be layered on top with the same behavior as the Windows application.

## Build locally

```bash
./scripts/Build-Debug.sh
```

If `cmake` or the Qt6 development packages are missing, the script installs them automatically before building.

The executable is generated in:

```text
artifacts/build/debug/src/SimpleTunnelSSH.App/SimpleTunnelSSH
```

## Build a `.deb` package

```bash
./scripts/Publish-Package.sh
```

or directly:

```bash
./scripts/Publish-Deb.sh
```

The Debian package is generated in:

```text
artifacts/installer/
```

On desktop environments such as Ubuntu, Zorin OS, and other Debian-based systems, the resulting `.deb` can be installed with a double click through the system software installer.

For terminal installation with automatic dependency resolution, use:

```bash
sudo apt install ./simpletunnelssh-*.deb
```

## End-user install flow

The intended end-user flow is:

1. Download the prebuilt `.deb` from GitHub Releases.
2. Install it with two clicks in the desktop software installer, or with `sudo apt install ./file.deb`.
3. Let the package manager fetch runtime dependencies such as Qt6 runtime packages and `openssh-client` automatically.

The end user should not need `cmake`, compilers, or Qt development packages. Those are build-time dependencies only.

## GitHub release packaging

The repository now includes a GitHub Actions workflow that builds the Linux `.deb` on Ubuntu and uploads it as a workflow artifact. On a GitHub Release publish event, it also attaches the generated `.deb` to the release.

## About `installer/simpletunnelssh.desktop.in`

That file is not the installer and it is not an `.ini` package file.

- It is a CMake template for the Linux `.desktop` launcher entry.
- During the build, CMake converts it into a real `.desktop` file.
- The actual installable artifact is the generated `.deb` package in `artifacts/installer/`.

## Runtime notes

- The application uses the system `ssh` binary from `openssh-client`.
- Startup integration is implemented through `~/.config/autostart/simpletunnelssh.desktop`.
- Configuration import is compatible with the Windows JSON structure used by the existing app.