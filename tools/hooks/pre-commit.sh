#!/bin/sh
#
# Run clang-format and ensure newline at EOF on staged files.
# Warns and skips formatting when clang-format is not on PATH.
# This file is version-controlled. Install it into .git/hooks with
#   tools/install-hooks.sh
#

staged_files=$(git diff --cached --name-only --diff-filter=ACM)
[ -z "$staged_files" ] && exit 0

need_fmt=
for file in $staged_files; do
    echo "$file" | grep -qE "^(third_party|assets|packs)/" && continue
    case "$file" in
        *.c|*.cpp|*.h|*.hpp) need_fmt=1; break ;;
    esac
done

# ---------- clang-format ----------
if [ -n "$need_fmt" ] && ! command -v clang-format >/dev/null 2>&1; then
    echo "WARNING: clang-format is not on PATH; staged C/C++ is not formatted." >&2
    need_fmt=
fi
if [ -n "$need_fmt" ]; then
    fmt_changed=0
    for file in $staged_files; do
        # Skip vendored code and assets
        echo "$file" | grep -qE "^(third_party|assets|packs)/" && continue
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
        echo "clang-format modified some files; please commit again"; exit 1; }
fi

# ---------- newline at EOF ----------
files_fixed=0
for file in $staged_files; do
    echo "$file" | grep -qE "^(third_party|assets|packs)/" && continue
    [ ! -f "$file" ] && continue
    [ ! -s "$file" ] && continue
    # Binary files (git reports "-" diff stats) keep their exact bytes.
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
[ $files_fixed -gt 0 ] && { echo "Added newlines to $files_fixed file(s); please commit again"; exit 1; }

echo "Pre-commit checks passed"
exit 0
