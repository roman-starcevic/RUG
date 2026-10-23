#!/usr/bin/env bash
# Run from a built RUG checkout. CXXFLAGS must match sanitizer instrumentation.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
experiment=$(mktemp -d)
trap 'rm -rf "$experiment"' EXIT
compiler=${CXX:-c++}
read -r -a flags <<< "${CXXFLAGS:--Wall -Wextra -Werror -std=c++98}"
"$compiler" "${flags[@]}" -I"$root" "$root/tests/empty-grammar.cpp" "$root/librug.a" -o "$experiment/regression"
: > "$experiment/empty.abnf"
printf '; no rules\n' > "$experiment/comment-only.abnf"
printf 'start = (\n' > "$experiment/malformed.abnf"
for name in empty comment-only malformed; do
  "$experiment/regression" "$experiment/$name.abnf"
done
