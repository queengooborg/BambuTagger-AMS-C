#!/bin/sh

set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mode=format

if [ "${1-}" = "--check" ]; then
  mode=check
elif [ "${1-}" != "" ]; then
  printf 'Usage: %s [--check]\n' "$0" >&2
  exit 2
fi

if ! command -v clang-format >/dev/null 2>&1; then
  printf '%s\n' 'clang-format is required. Install it with Homebrew or your system package manager.' >&2
  exit 1
fi

set -- $(
  cd "$repo_root"
  git ls-files -- '*.cpp' '*.h' | while IFS= read -r file; do
    if [ -f "$file" ]; then
      printf '%s\n' "$file"
    fi
  done
)

if [ "$#" -eq 0 ]; then
  printf '%s\n' 'No maintained C++ files found.'
  exit 0
fi

cd "$repo_root"
if [ "$mode" = check ]; then
  clang-format --dry-run --Werror "$@"
else
  clang-format -i "$@"
fi