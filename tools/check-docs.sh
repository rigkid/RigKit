#!/usr/bin/env bash
# Doxygen output gate.
# A warning means text is missing from the HTML. A missing symbol means a
# documented namespace-scope function or enum was dropped, which Doxygen does
# not warn about. Pack Pages runs this same script.
#
# Host, after `cmake --build build --target docs`:
#   ./tools/check-docs.sh build
# One pack, after generate-pack-docs.sh:
#   ./tools/check-docs.sh --out build/docs/pack/rigSvg --input packs/rigSvg --exclude-examples
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

OUT=""
INPUTS=()
EXCLUDE_EXAMPLES=0

if [[ $# -eq 0 || ( $# -eq 1 && "$1" != --* ) ]]; then
	BUILD="${1:-build}"
	OUT="$BUILD/docs/api"
	INPUTS=(src packs)
else
	while [[ $# -gt 0 ]]; do
		case "$1" in
			--out)
				OUT="${2:-}"
				shift 2
				;;
			--input)
				INPUTS+=("${2:-}")
				shift 2
				;;
			--exclude-examples)
				EXCLUDE_EXAMPLES=1
				shift
				;;
			*)
				echo "Unknown argument: $1" >&2
				exit 1
				;;
		esac
	done
fi

if [[ -z "$OUT" || ${#INPUTS[@]} -eq 0 ]]; then
	echo "Usage: $0 [cmakeBuildDir]" >&2
	echo "       $0 --out <doxyOut> --input <tree> [--exclude-examples]" >&2
	exit 1
fi

WARN="$OUT/doxygen.warn"
XML="$OUT/xml"

if [ ! -f "$WARN" ]; then
	echo "FAIL: no $WARN — run the docs build first" >&2
	exit 1
fi

if [ -s "$WARN" ]; then
	echo "FAIL: Doxygen reported warnings ($(wc -l <"$WARN" | tr -d ' ') lines):" >&2
	cat "$WARN" >&2
	echo "" >&2
	echo "A dot before punctuation ends the brief and splits any code span around" >&2
	echo "it: escape the dot outside backticks — @c rig\\.* , not \`rig.*\`." >&2
	echo "Wrap plain <angle brackets> in backticks. Give @ref a target Doxygen" >&2
	echo "emits: free functions need the full namespace (@ref rigkit::plot::foo)," >&2
	echo "and an undocumented member takes backticks instead." >&2
	echo "See skills/rigkit-comments/SKILL.md." >&2
	exit 1
fi

echo "OK: Doxygen warning log is empty"

if command -v python3 >/dev/null 2>&1; then
	PY=python3
elif command -v python >/dev/null 2>&1; then
	PY=python
else
	echo "FAIL: python3 not found; cannot check documented symbols" >&2
	exit 1
fi

ARGS=(tools/check_doc_symbols.py --xml "$XML")
for input in "${INPUTS[@]}"; do
	ARGS+=(--input "$input")
done
if [[ "$EXCLUDE_EXAMPLES" -eq 1 ]]; then
	ARGS+=(--exclude-examples)
fi
"$PY" "${ARGS[@]}"
