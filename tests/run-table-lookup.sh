#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
experiment=$(mktemp -d)
trap 'rm -rf "$experiment"' EXIT
compiler=${CXX:-c++}
read -r -a flags <<< "${CXXFLAGS:--Wall -Wextra -Werror -std=c++98}"
"$compiler" "${flags[@]}" -I"$root" "$root/tests/table-lookup.cpp" "$root/librug.a" -o "$experiment/regression"
"$experiment/regression"
