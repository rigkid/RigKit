#!/usr/bin/env bash
# Doxygen must finish with an empty warning log.
# A warning here means real text is missing from the generated HTML: an
# unbackticked angle bracket truncates the rest of its comment block, and an
# unresolved @ref renders as plain prose. Build the docs first:
#   cmake -S . -B build && cmake --build build --target docs
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD="${1:-build}"
WARN="$BUILD/docs/api/doxygen.warn"

if [ ! -f "$WARN" ]; then
	echo "FAIL: no $WARN — run: cmake --build $BUILD --target docs" >&2
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
	echo "and an undocumented member or constexpr takes backticks instead." >&2
	echo "See skills/rigkit-comments/SKILL.md." >&2
	exit 1
fi

echo "OK: Doxygen warning log is empty"
