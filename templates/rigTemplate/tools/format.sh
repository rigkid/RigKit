#!/usr/bin/env bash
# Format first-party C/C++ (git-tracked and untracked, skips third_party).
# One source of truth for every platform - on Windows run from Git Bash.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v clang-format >/dev/null 2>&1; then
	echo "clang-format is not on PATH; nothing formatted." >&2
	exit 1
fi

mapfile -t FILES < <(git ls-files --cached --others --exclude-standard '*.c' '*.cpp' '*.h' '*.hpp' | grep -vE '(^|/)third_party/' || true)

if [ "${#FILES[@]}" -eq 0 ]; then
	echo "No source files to format."
	exit 0
fi

echo "clang-format ($(clang-format --version | head -n1)) on ${#FILES[@]} files..."
clang-format -i "${FILES[@]}"
echo "Done."
