#!/usr/bin/env bash
# Publish templates/rigTemplate → https://github.com/rigkid/rigTemplate.git
# Pushes the template remote only — not the RigKit host repo.
#
# Usage:
#   ./tools/publish-template.sh
#   ./tools/publish-template.sh --dry-run
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

DRY=0
if [[ "${1:-}" == "--dry-run" ]]; then
	DRY=1
fi

ORG="${RIGKIT_ADDON_ORG:-rigkid}"
NAME="rigTemplate"
SRC="$ROOT/templates/${NAME}"
URL="https://github.com/${ORG}/${NAME}.git"

need() {
	command -v "$1" >/dev/null 2>&1 || {
		echo "Missing required tool: $1" >&2
		exit 1
	}
}
need gh
need git

if [[ ! -d "$SRC" ]]; then
	echo "Missing $SRC" >&2
	exit 1
fi

echo "=== ${NAME} ==="
if [[ "$DRY" -eq 1 ]]; then
	echo "  would ensure repo ${ORG}/${NAME}"
	echo "  would push $SRC → $URL (main)"
	exit 0
fi

if ! gh repo view "${ORG}/${NAME}" >/dev/null 2>&1; then
	echo "  creating ${ORG}/${NAME}"
	gh repo create "${ORG}/${NAME}" --public --description "RigKit pack scaffold"
else
	echo "  repo exists"
fi

tmp="$(mktemp -d)"
# Prefer committed tree; fall back to working copy rsync-style copy
if git -C "$ROOT" cat-file -e "HEAD:templates/${NAME}/pack.json" 2>/dev/null; then
	git -C "$ROOT" archive HEAD "templates/${NAME}" | tar -x -C "$tmp"
	export_dir="$tmp/templates/${NAME}"
else
	mkdir -p "$tmp/templates"
	cp -a "$SRC" "$tmp/templates/${NAME}"
	export_dir="$tmp/templates/${NAME}"
fi

git -C "$export_dir" init -q -b main
git -C "$export_dir" add -A
git -C "$export_dir" -c user.email="noreply@rigkit.dev" -c user.name="RigKit" \
	commit -q -m "Publish ${NAME} scaffold from RigKit host."
git -C "$export_dir" remote add origin "$URL"
git -C "$export_dir" push -u origin main

rm -rf "$tmp"
echo "  pushed $URL"
echo "  Apps copy/clone this scaffold into packs/<newName>/ and rename."
