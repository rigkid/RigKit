#!/usr/bin/env bash
# Generate pack-scoped Doxygen HTML (no CMake configure required).
#
# Usage:
#   ./tools/generate-pack-docs.sh <packName> [hostRoot] [outDir]
#
# Defaults: hostRoot = repo root, outDir = <hostRoot>/build/docs/pack/<packName>
# Output: <outDir>/html/index.html
set -euo pipefail

PACK_NAME="${1:-}"
if [[ -z "$PACK_NAME" ]]; then
	echo "Usage: $0 <packName> [hostRoot] [outDir]" >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEFAULT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RIGKIT_ROOT="$(cd "${2:-$DEFAULT_ROOT}" && pwd)"

if [[ -n "${3:-}" ]]; then
	mkdir -p "$3"
	RIGKIT_DOXY_OUT="$(cd "$3" && pwd)"
else
	RIGKIT_DOXY_OUT="$RIGKIT_ROOT/build/docs/pack/$PACK_NAME"
	mkdir -p "$RIGKIT_DOXY_OUT"
	RIGKIT_DOXY_OUT="$(cd "$RIGKIT_DOXY_OUT" && pwd)"
fi

PACK_DIR="$RIGKIT_ROOT/packs/$PACK_NAME"
if [[ ! -d "$PACK_DIR" ]]; then
	echo "Pack tree missing: $PACK_DIR" >&2
	exit 1
fi

if ! command -v doxygen >/dev/null 2>&1; then
	echo "doxygen not found on PATH" >&2
	exit 1
fi

TEMPLATE="$RIGKIT_ROOT/docs/api/PackDoxyfile.in"
if [[ ! -f "$TEMPLATE" ]]; then
	echo "Missing $TEMPLATE" >&2
	exit 1
fi

PROJECT_VERSION="0.1.0"
if [[ -f "$PACK_DIR/pack.json" ]] && command -v python >/dev/null 2>&1; then
	PROJECT_VERSION="$(python - "$PACK_DIR/pack.json" <<'PY'
import json, sys
with open(sys.argv[1], encoding="utf-8") as f:
	print(json.load(f).get("version") or "0.1.0")
PY
)"
fi

RIGKIT_DOXY_HAVE_DOT=NO
if command -v dot >/dev/null 2>&1; then
	RIGKIT_DOXY_HAVE_DOT=YES
fi

mkdir -p "$RIGKIT_DOXY_OUT"
cat >"$RIGKIT_DOXY_OUT/mainpage.md" <<EOF
# ${PACK_NAME} API reference {#mainpage}

Pack-scoped Doxygen from public headers (\`@brief\`, \`@param\`, \`@return\`, …).

Host aggregate (all packs + core): [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/)

Narrative docs stay in the RigKit repo (\`docs/\`, \`AGENTS.md\`).
EOF

# Native Doxygen on Windows needs D:/... not Git Bash /d/...
doxy_path() {
	local p="$1"
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -m "$p"
	elif [[ -d "$p" ]] && (cd "$p" && pwd -W >/dev/null 2>&1); then
		(cd "$p" && pwd -W | sed 's|\\|/|g')
	else
		echo "${p//\\//}"
	fi
}

root_esc="$(doxy_path "$RIGKIT_ROOT")"
out_esc="$(doxy_path "$RIGKIT_DOXY_OUT")"

DOXYFILE="$RIGKIT_DOXY_OUT/Doxyfile"
sed \
	-e "s|@PACK_NAME@|${PACK_NAME}|g" \
	-e "s|@PROJECT_VERSION@|${PROJECT_VERSION}|g" \
	-e "s|@RIGKIT_ROOT@|${root_esc}|g" \
	-e "s|@RIGKIT_DOXY_OUT@|${out_esc}|g" \
	-e "s|@RIGKIT_DOXY_HAVE_DOT@|${RIGKIT_DOXY_HAVE_DOT}|g" \
	"$TEMPLATE" >"$DOXYFILE"

echo "Doxygen → $RIGKIT_DOXY_OUT/html (pack=$PACK_NAME)"
doxygen "$DOXYFILE"
if [[ ! -f "$RIGKIT_DOXY_OUT/html/index.html" ]]; then
	echo "Doxygen did not produce html/index.html" >&2
	exit 1
fi
echo "Done: $RIGKIT_DOXY_OUT/html/index.html"
