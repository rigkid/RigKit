#!/usr/bin/env bash
# Report local pack checkout health and optional app.json pin drift.
# Pair with tools/update-packs.sh to refresh checkouts.
#
# Usage:
#   ./tools/pack-status.sh
#   ./tools/pack-status.sh --app examples/svgEditor/app.json
#   ./tools/pack-status.sh --build examples/svgEditor/build
#   ./tools/pack-status.sh --strict          # exit 1 on dirty / pin drift / bad RIGKIT_DIR
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

APP_MANIFEST=""
BUILD_DIR=""
STRICT=0

while [[ $# -gt 0 ]]; do
	case "$1" in
	--app)
		APP_MANIFEST="$2"
		shift 2
		;;
	--build)
		BUILD_DIR="$2"
		shift 2
		;;
	--strict) STRICT=1; shift ;;
	-h | --help)
		sed -n '2,14p' "$0"
		exit 0
		;;
	*)
		echo "Unknown arg: $1" >&2
		exit 1
		;;
	esac
done

FAIL=0

note_fail() {
	FAIL=1
}

json_get() {
	python - "$1" "$2" <<'PY'
import json, sys
path = sys.argv[2].split(".")
with open(sys.argv[1], encoding="utf-8") as f:
    data = json.load(f)
cur = data
for p in path:
    if p.endswith("]"):
        name, idx = p[:-1].split("[")
        cur = cur[name][int(idx)]
    else:
        cur = cur.get(p, "")
print(cur if cur is not None else "")
PY
}

canonical_dir() {
	python - "$1" <<'PY'
import os, sys
print(os.path.normcase(os.path.normpath(os.path.abspath(sys.argv[1]))))
PY
}

pack_git_state() {
	local dir="$1"
	if [[ ! -d "$dir/.git" && ! -f "$dir/.git" ]]; then
		echo "plain"
		return 0
	fi
	if [[ -n "$(git -C "$dir" status --porcelain 2>/dev/null)" ]]; then
		echo "dirty"
		return 0
	fi
	local upstream
	upstream="$(git -C "$dir" rev-parse --abbrev-ref '@{u}' 2>/dev/null || true)"
	if [[ -z "$upstream" ]]; then
		echo "clean(no-upstream)"
		return 0
	fi
	local ahead behind
	read -r ahead behind < <(git -C "$dir" rev-list --left-right --count "@{u}...HEAD" 2>/dev/null || echo "0 0")
	if [[ "$ahead" -gt 0 && "$behind" -gt 0 ]]; then
		echo "diverged(+${ahead}/-${behind})"
	elif [[ "$ahead" -gt 0 ]]; then
		echo "ahead(${ahead})"
	elif [[ "$behind" -gt 0 ]]; then
		echo "behind(${behind})"
	else
		echo "clean"
	fi
}

print_pack_row() {
	local name="$1"
	local pin_ref="$2"
	local dir="$ROOT/packs/$name"
	local source="missing"
	local head="-"
	local state="-"

	if [[ -d "$dir" ]]; then
		if [[ -d "$dir/.git" || -f "$dir/.git" ]]; then
			source="local"
			head="$(git -C "$dir" rev-parse --short HEAD 2>/dev/null || echo "?")"
			state="$(pack_git_state "$dir")"
		else
			source="tree"
			state="not-git"
		fi
	else
		source="cpm"
		state="absent"
	fi

	printf "%-22s %-8s %-12s %-8s %s\n" "$name" "$source" "${pin_ref:-main}" "$head" "$state"

	if [[ "$STRICT" -eq 1 ]]; then
		case "$state" in
		dirty | diverged* | behind*)
			note_fail
			;;
		esac
		if [[ "$source" == "local" && -n "$pin_ref" ]]; then
			if ! git -C "$dir" merge-base --is-ancestor "$pin_ref" HEAD 2>/dev/null &&
				! git -C "$dir" rev-parse -q --verify "$pin_ref" >/dev/null 2>&1; then
				:
			fi
		fi
	fi
}

echo "RigKit root: $ROOT"
echo

if [[ -n "$BUILD_DIR" ]]; then
	if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
		echo "[build] no CMakeCache.txt at $BUILD_DIR" >&2
		note_fail
	else
		cached="$(grep '^RIGKIT_DIR:PATH=' "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2- || true)"
		if [[ -z "$cached" ]]; then
			echo "[build] RIGKIT_DIR not in cache (legacy configure?)"
		else
			root_canon="$(canonical_dir "$ROOT")"
			cached_canon="$(canonical_dir "$cached")"
			echo "[build] $BUILD_DIR"
			echo "        RIGKIT_DIR=$cached"
			if [[ "$root_canon" == "$cached_canon" ]]; then
				echo "        OK — build uses this RigKit checkout"
			else
				echo "        WARN — build points elsewhere (expected $ROOT)" >&2
				note_fail
			fi
		fi
	fi
	echo
fi

declare -A PINNED=()

if [[ -n "$APP_MANIFEST" ]]; then
	if [[ ! -f "$APP_MANIFEST" ]]; then
		echo "Manifest not found: $APP_MANIFEST" >&2
		exit 1
	fi
	echo "App manifest: $APP_MANIFEST"
	count="$(python - "$APP_MANIFEST" <<'PY'
import json, sys
print(len(json.load(open(sys.argv[1], encoding="utf-8")).get("dependencies", [])))
PY
)"
	i=0
	while [[ "$i" -lt "$count" ]]; do
		name="$(json_get "$APP_MANIFEST" "dependencies[$i].name")"
		ref="$(json_get "$APP_MANIFEST" "dependencies[$i].ref")"
		if [[ -z "$ref" ]]; then
			ref="$(json_get "$APP_MANIFEST" "dependencies[$i].branch")"
		fi
		PINNED["$name"]="${ref:-main}"
		i=$((i + 1))
	done
	echo
fi

echo "Pack checkouts under packs/:"
printf "%-22s %-8s %-12s %-8s %s\n" "NAME" "SOURCE" "PIN(ref)" "HEAD" "STATE"
printf "%-22s %-8s %-12s %-8s %s\n" "----" "------" "--------" "----" "-----"

if [[ ${#PINNED[@]} -gt 0 ]]; then
	for name in $(printf '%s\n' "${!PINNED[@]}" | sort); do
		print_pack_row "$name" "${PINNED[$name]}"
	done
else
	for dir in "$ROOT"/packs/*; do
		[[ -d "$dir" ]] || continue
		name="$(basename "$dir")"
		ref=""
		if [[ -f "$dir/pack.json" ]]; then
			ref="$(json_get "$dir/pack.json" ref)"
			if [[ -z "$ref" ]]; then
				ref="$(json_get "$dir/pack.json" branch)"
			fi
		fi
		print_pack_row "$name" "${ref:-main}"
	done
fi

echo
echo "Tips:"
echo "  Refresh checkouts:  ./tools/update-packs.sh [--app <app.json>] [--latest]"
echo "  Examples expect:    RIGKIT_DIR=\${CMAKE_CURRENT_SOURCE_DIR}/../..  (in-tree)"
echo "  Sibling apps:       RIGKIT_DIR=\${CMAKE_CURRENT_SOURCE_DIR}/../RigKit"

exit "$FAIL"
