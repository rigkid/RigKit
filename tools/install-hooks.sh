#!/usr/bin/env bash
# Install git hooks for this clone.
# From the RigKit host: host hook + every packs/<name> that is its own git repo.
# From a pack: that pack only (uses tools/hooks/pre-commit.sh if present).
# Usage: ./tools/install-hooks.sh   or   tools\install-hooks.bat
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"

install_hook() {
	local repo="$1"
	local src="$2"
	local dest
	dest="$(git -C "$repo" rev-parse --git-path hooks)/pre-commit"
	mkdir -p "$(dirname "$dest")"
	cp "$src" "$dest"
	chmod +x "$dest"
	echo "Installed $(basename "$repo") -> $dest"
}

HOST_HOOK="$REPO_ROOT/tools/hooks/pre-commit.sh"
PACK_HOOK="$REPO_ROOT/tools/hooks/pack-pre-commit.sh"
LOCAL_PACK_HOOK="$REPO_ROOT/tools/hooks/pre-commit.sh"

if [[ -f "$REPO_ROOT/pack.json" && ! -f "$REPO_ROOT/cmake/RigKitPacks.cmake" ]]; then
	# Pack checkout (standalone or packs/<name>).
	if [[ -f "$LOCAL_PACK_HOOK" ]]; then
		install_hook "$REPO_ROOT" "$LOCAL_PACK_HOOK"
	elif [[ -f "$PACK_HOOK" ]]; then
		install_hook "$REPO_ROOT" "$PACK_HOOK"
	else
		echo "No pack pre-commit hook at $LOCAL_PACK_HOOK" >&2
		exit 1
	fi
	exit 0
fi

if [[ ! -f "$HOST_HOOK" ]]; then
	echo "Missing $HOST_HOOK" >&2
	exit 1
fi
install_hook "$REPO_ROOT" "$HOST_HOOK"

if [[ ! -f "$PACK_HOOK" ]]; then
	echo "No pack hook at $PACK_HOOK; host only."
	exit 0
fi

installed=0
skipped=0
for pack_json in "$REPO_ROOT"/packs/*/pack.json; do
	[[ -f "$pack_json" ]] || continue
	pack_dir="$(dirname "$pack_json")"
	if ! git -C "$pack_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
		skipped=$((skipped + 1))
		continue
	fi
	pack_top="$(git -C "$pack_dir" rev-parse --show-toplevel)"
	# Normalize for Windows path compare (git may return mixed slashes).
	if [[ "$(cd "$pack_top" && pwd)" = "$(cd "$REPO_ROOT" && pwd)" ]]; then
		echo "Skip $pack_dir (not a separate git repo)"
		skipped=$((skipped + 1))
		continue
	fi
	src="$PACK_HOOK"
	if [[ -f "$pack_dir/tools/hooks/pre-commit.sh" ]]; then
		src="$pack_dir/tools/hooks/pre-commit.sh"
	fi
	install_hook "$pack_dir" "$src"
	installed=$((installed + 1))
done

echo "Pack hooks: $installed installed, $skipped skipped (no git / host-owned tree)."
