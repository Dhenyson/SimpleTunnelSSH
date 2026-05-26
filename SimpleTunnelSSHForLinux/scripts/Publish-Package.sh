#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "$script_dir/.." && pwd)"
build_dir="$project_dir/artifacts/build/release"
package_dir="$project_dir/artifacts/installer"

"$script_dir/Ensure-Build-Dependencies.sh"

cmake -S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$build_dir" --parallel
mkdir -p "$package_dir"
cpack --config "$build_dir/CPackConfig.cmake" -B "$package_dir"