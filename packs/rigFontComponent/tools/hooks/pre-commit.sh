#!/bin/sh
#
# Pack pre-commit: clang-format (SortIncludes) + newline at EOF on staged files.
# Skips third_party/ and build/. Install: ./tools/install-hooks.sh
#

command_exists() { command -v "$1" >/dev/null 2>&1; }

cd "$(git rev-parse --show-toplevel)" || exit 1

staged_files=$(git diff --cached --name-only --diff-filter=ACM)
[ -z "$staged_files" ] && exit 0

# ---------- clang-format ----------
if command_exists clang-format; then
	fmt_changed=0
	for file in $staged_files; do
		echo "$file" | grep -qE '(^|/)(third_party|build)/' && continue
		case "$file" in
			*.c|*.cpp|*.h|*.hpp)
				clang-format -i "$file"
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
