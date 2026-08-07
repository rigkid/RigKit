#!/usr/bin/env bash
# Refresh local pack checkouts / submodules to a pinned ref.
# Does not run inside the Canvas host — configure-time / developer tooling only.
#
# Usage:
#   ./tools/update-packs.sh              # in-org packs → pack.json ref
#   ./tools/update-packs.sh --latest     # fetch origin/main (or ref) and print SHAs
#   ./tools/update-packs.sh --app examples/oscHost/app.json
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

LATEST=0
APP_MANIFEST=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	--latest) LATEST=1; shift ;;
	--app)
		APP_MANIFEST="$2"
		shift 2
		;;
	-h|--help)
		sed -n '2,12p' "$0"
		exit 0
		;;
	*)
		echo "Unknown arg: $1" >&2
		exit 1
		;;
	esac
done

json_get() {
	# json_get <file> <jq-ish path using python>
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

checkout_pack() {
	local dir="$1"
	local ref="$2"
	local name
	name="$(basename "$dir")"
	if [[ ! -d "$dir" ]]; then
		echo "[skip] $name — not present under packs/"
		return 0
	fi
	if [[ ! -d "$dir/.git" && ! -f "$dir/.git" ]]; then
		echo "[skip] $name — not a git checkout/submodule (plain working tree)"
		return 0
	fi
	if [[ -z "$ref" ]]; then
		ref="main"
	fi
	echo "[update] $name → $ref"
	git -C "$dir" fetch --tags origin 2>/dev/null || git -C "$dir" fetch --tags 2>/dev/null || true
	if [[ "$LATEST" -eq 1 ]]; then
		git -C "$dir" checkout -q "$ref"
		git -C "$dir" pull --ff-only 2>/dev/null || true
	else
		git -C "$dir" checkout -q "$ref"
	fi
	echo "         $(git -C "$dir" rev-parse --short HEAD) $(git -C "$dir" rev-parse HEAD)"
}

INORG=(rigComponent rigSystems rigProject rigImGui)

for name in "${INORG[@]}"; do
	dir="$ROOT/packs/$name"
	ref=""
	if [[ -f "$dir/pack.json" ]]; then
		ref="$(json_get "$dir/pack.json" ref)"
		if [[ -z "$ref" ]]; then
			ref="$(json_get "$dir/pack.json" branch)"
		fi
	fi
	checkout_pack "$dir" "$ref"
done

if [[ -n "$APP_MANIFEST" ]]; then
	if [[ ! -f "$APP_MANIFEST" ]]; then
		echo "Manifest not found: $APP_MANIFEST" >&2
		exit 1
	fi
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
		# Skip in-org packs already handled
		skip=0
		for d in "${INORG[@]}"; do
			[[ "$name" == "$d" ]] && skip=1 && break
		done
		if [[ "$skip" -eq 0 ]]; then
			checkout_pack "$ROOT/packs/$name" "$ref"
		fi
		i=$((i + 1))
	done
fi

echo "Done. Basics are submodules — commit new SHAs in RigKit when you bump them."
echo "Optional packs: clone under packs/<name>/ or let CPM fetch from app.json url+ref."
