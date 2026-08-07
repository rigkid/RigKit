#!/usr/bin/env bash
# Simple helper to install project git hooks.
# Usage: ./tools/install-hooks.sh
set -euo pipefail
REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOK_SRC="$REPO_ROOT/tools/hooks/pre-commit.sh"
HOOK_DEST="$REPO_ROOT/.git/hooks/pre-commit"
cp "$HOOK_SRC" "$HOOK_DEST"
chmod +x "$HOOK_DEST"
echo "Installed pre-commit hook from $HOOK_SRC" 
