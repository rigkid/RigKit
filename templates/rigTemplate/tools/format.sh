#!/usr/bin/env bash
# Format first-party C/C++ (src, examples). Skips third_party and build.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v clang-format >/dev/null 2>&1; then
	echo "clang-format not found on PATH" >&2
	exit 1
fi

mapfile -t FILES < <(find src examples -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) 2>/dev/null | grep -vE '/(third_party|build)/' || true)

if [ "${#FILES[@]}" -eq 0 ]; then
	echo "No source files to format."
	exit 0
fi

echo "clang-format ($(clang-format --version | head -n1)) on ${#FILES[@]} files..."
clang-format -i "${FILES[@]}"
echo "Done."
