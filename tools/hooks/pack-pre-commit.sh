#!/bin/sh
#
# Pack pre-commit: clang-format (SortIncludes) + newline at EOF on staged files.
# Skips third_party/ and build/. Fails closed if C++ is staged and clang-format
# is missing (silent skip used to let unformatted commits reach pack CI).
# Install from the pack or from the RigKit host:
#   ./tools/install-hooks.sh
#   tools\install-hooks.bat
#

command_exists() { command -v "$1" >/dev/null 2>&1; }

find_clang_format() {
	if command_exists clang-format; then
		command -v clang-format
		return 0
	fi
	# Common Windows / LLVM installs (Git Bash hooks often lack PATH).
	# Keep in sync with packs/*/tools/format.sh candidate list.
	for c in \
		"/c/Program Files/LLVM/bin/clang-format.exe" \
		"/c/Program Files (x86)/LLVM/bin/clang-format.exe" \
		"$LOCALAPPDATA/Programs/LLVM/bin/clang-format.exe" \
		"/c/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-format.exe" \
		"/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format.exe" \
		"/c/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format.exe" \
		"/c/msys64/ucrt64/bin/clang-format.exe"
	do
		if [ -x "$c" ]; then
			echo "$c"
			return 0
		fi
	done
	return 1
}

cd "$(git rev-parse --show-toplevel)" || exit 1

staged_files=$(git diff --cached --name-only --diff-filter=ACM)
[ -z "$staged_files" ] && exit 0

need_fmt=
for file in $staged_files; do
	echo "$file" | grep -qE '(^|/)(third_party|build)/' && continue
	case "$file" in
		*.c|*.cpp|*.h|*.hpp) need_fmt=1; break ;;
	esac
done

CLANG_FORMAT=
if [ -n "$need_fmt" ]; then
	if ! CLANG_FORMAT="$(find_clang_format)"; then
		echo "FAIL: clang-format required to commit C/C++ in this pack." >&2
		echo "Install LLVM clang-format and put it on PATH, then retry." >&2
		echo "See README Style (or RigKit packs/README.md#style)." >&2
		echo "Or format first: ./tools/format.sh" >&2
		exit 1
	fi
fi

# ---------- clang-format ----------
if [ -n "$CLANG_FORMAT" ]; then
	fmt_changed=0
	for file in $staged_files; do
		echo "$file" | grep -qE '(^|/)(third_party|build)/' && continue
		case "$file" in
			*.c|*.cpp|*.h|*.hpp)
				"$CLANG_FORMAT" -i "$file"
				if ! git diff --quiet "$file"; then
					git add "$file"
					fmt_changed=1
				fi
				;;
		esac
	done
	[ $fmt_changed -gt 0 ] && {
		echo "clang-format modified some files; please commit again"
		exit 1
	}
fi

# ---------- newline at EOF ----------
files_fixed=0
for file in $staged_files; do
	echo "$file" | grep -qE '(^|/)(third_party|build)/' && continue
	[ ! -f "$file" ] && continue
	[ ! -s "$file" ] && continue
	case "$(git diff --cached --numstat -- "$file" | cut -f1)" in
		-|"") continue ;;
	esac
	if [ "$(tail -c1 "$file" | wc -l)" -eq 0 ]; then
		echo "Adding newline to end of file: $file"
		echo "" >> "$file"
		git add "$file"
		files_fixed=$((files_fixed + 1))
	fi
done
[ $files_fixed -gt 0 ] && {
	echo "Added newlines to $files_fixed file(s); please commit again"
	exit 1
}

echo "Pre-commit checks passed"
exit 0
