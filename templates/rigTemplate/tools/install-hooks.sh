#!/usr/bin/env bash
# Install this pack's pre-commit hook (clang-format + newline at EOF).
# Usage: ./tools/install-hooks.sh
set -euo pipefail
REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOK_SRC="$REPO_ROOT/tools/hooks/pre-commit.sh"
DEST="$(git -C "$REPO_ROOT" rev-parse --git-path hooks)/pre-commit"
mkdir -p "$(dirname "$DEST")"
cp "$HOOK_SRC" "$DEST"
chmod +x "$DEST"
echo "Installed $DEST"
