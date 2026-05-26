#!/usr/bin/env bash
set -euo pipefail

packages=(
  build-essential
  cmake
  qt6-base-dev
  qt6-base-dev-tools
  qt6-tools-dev-tools
  pkg-config
  dpkg-dev
  fakeroot
  qt6-qpa-plugins
  openssh-client
)

if ! command -v apt-get >/dev/null 2>&1; then
  echo "Automatic build dependency installation is currently supported only on Debian, Ubuntu, and Zorin OS." >&2
  exit 1
fi

if [[ "$(id -u)" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    exec sudo --preserve-env=DEBIAN_FRONTEND bash "$0" "$@"
  fi

  echo "This script needs root privileges to install packages. Run it with sudo." >&2
  exit 1
fi

export DEBIAN_FRONTEND="${DEBIAN_FRONTEND:-noninteractive}"

echo "Installing Linux build dependencies for SimpleTunnelSSH..."
apt-get update
apt-get install -y --no-install-recommends "${packages[@]}"
echo "Build dependencies installed successfully."