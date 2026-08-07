#!/usr/bin/env bash
# Ensure first-party pack trees exist under packs/ for host aggregate Doxygen.
# Reads docs/api/pack-remotes.txt; skips clone failures with a warning.
#
# Usage (from host root, after checkout + submodules):
#   ./tools/fetch-packs-for-docs.sh
#
# Auth: RIGKIT_CI_TOKEN or GITHUB_TOKEN when set (https rewrite for github.com).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

LIST="$ROOT/docs/api/pack-remotes.txt"
if [[ ! -f "$LIST" ]]; then
	echo "Missing $LIST" >&2
	exit 1
fi

TOKEN="${RIGKIT_CI_TOKEN:-${GITHUB_TOKEN:-}}"

clone_url() {
	local url="$1"
	if [[ -n "$TOKEN" && "$url" =~ ^https://github.com/ ]]; then
		echo "https://x-access-token:${TOKEN}@github.com/${url#https://github.com/}"
	else
		echo "$url"
	fi
}

pack_present() {
	local dir="$1"
	[[ -f "$dir/CMakeLists.txt" || -d "$dir/src" ]]
}

while IFS=$'\t' read -r name url ref || [[ -n "${name:-}" ]]; do
	[[ -z "${name:-}" || "$name" =~ ^# ]] && continue
	ref="${ref:-main}"
	dir="$ROOT/packs/$name"

	if pack_present "$dir"; then
		echo "[ok] $name"
		continue
	fi

	if [[ -z "${url:-}" ]]; then
		echo "[skip] $name — no url"
		continue
	fi

	auth_url="$(clone_url "$url")"
	echo "[fetch] $name ← $url @ $ref"
	tmp="$(mktemp -d)"
	ok=0
	if git clone --depth 1 --branch "$ref" "$auth_url" "$tmp/pack" 2>/dev/null; then
		ok=1
	elif git clone --depth 1 "$auth_url" "$tmp/pack" 2>/dev/null; then
		git -C "$tmp/pack" checkout -q "$ref" 2>/dev/null || true
		ok=1
	fi
	if [[ "$ok" -eq 1 ]]; then
		rm -rf "$dir"
		mkdir -p "$ROOT/packs"
		mv "$tmp/pack" "$dir"
		echo "         fetched $name"
	else
		echo "[warn] $name — clone failed (aggregate docs continue without it)" >&2
	fi
	rm -rf "$tmp"
done <"$LIST"

echo "fetch-packs-for-docs: done"
