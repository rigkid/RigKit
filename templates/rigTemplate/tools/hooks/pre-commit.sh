#!/bin/sh
#
# Pack pre-commit: clang-format (SortIncludes) + newline at EOF on staged files.
# Skips third_party/ and build/. Warns and skips formatting when clang-format is
# not on PATH.
# Install from the pack or from the RigKit host:
#   ./tools/install-hooks.sh
#

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

has_fmt=
if [ -n "$need_fmt" ]; then
	if command -v clang-format >/dev/null 2>&1; then
		has_fmt=1
	else
		echo "WARNING: clang-format is not on PATH; staged C/C++ is not formatted." >&2
	fi
fi

# ---------- clang-format ----------
if [ -n "$has_fmt" ]; then
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
