#!/usr/bin/env bash
# Format first-party C/C++ sources (src, examples, tools). Skips third_party and packs.
# One source of truth for every platform - on Windows run from Git Bash.
# Finds clang-format on PATH or in the usual LLVM / Visual Studio / MSYS2 spots.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CLANG_FORMAT="${CLANG_FORMAT:-}"
if [ -z "$CLANG_FORMAT" ] && command -v clang-format >/dev/null 2>&1; then
	CLANG_FORMAT="clang-format"
fi
if [ -z "$CLANG_FORMAT" ]; then
	for candidate in \
		"/c/Program Files/LLVM/bin/clang-format.exe" \
		"/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-format.exe" \
		"/c/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format.exe" \
		"/c/msys64/ucrt64/bin/clang-format.exe"; do
		if [ -x "$candidate" ]; then
			CLANG_FORMAT="$candidate"
			break
		fi
	done
fi
if [ -z "$CLANG_FORMAT" ]; then
	echo "clang-format not found. Install LLVM (or Visual Studio's LLVM tools) or add clang-format to PATH." >&2
	exit 1
fi

mapfile -t FILES < <(find src examples tools -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) 2>/dev/null | grep -vE '/build/' || true)

if [ "${#FILES[@]}" -eq 0 ]; then
	echo "No source files to format."
	exit 0
fi

echo "clang-format ($("$CLANG_FORMAT" --version | head -n1)) on ${#FILES[@]} files..."
"$CLANG_FORMAT" -i "${FILES[@]}"
echo "Done."
