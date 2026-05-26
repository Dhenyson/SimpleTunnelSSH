#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

required_commands=(cmake cpack pkg-config)
required_packages=(
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

missing_items=()

for command_name in "${required_commands[@]}"; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    missing_items+=("command:$command_name")
  fi
done

if command -v dpkg-query >/dev/null 2>&1; then
  for package_name in "${required_packages[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$package_name" 2>/dev/null | grep -q '^install ok installed$'; then
      missing_items+=("package:$package_name")
    fi
  done
fi

if [[ "${#missing_items[@]}" -eq 0 ]]; then
  exit 0
fi

if [[ "${STS_SKIP_DEPENDENCY_INSTALL:-0}" == "1" ]]; then
  printf 'Missing build dependencies:\n' >&2
  printf '  %s\n' "${missing_items[@]}" >&2
  exit 127
fi

echo "Missing build dependencies detected. Installing them automatically..."
"$script_dir/Install-Build-Dependencies.sh"

for command_name in "${required_commands[@]}"; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "Dependency bootstrap did not provide required command: $command_name" >&2
    exit 127
  fi
done