#!/usr/bin/env bash
# Machine-checkable slices of the Ten Commandments.
# See docs/contract/commandments.md — keep this script small; do not turn it into a wiki.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

FAIL=0
hit() {
	echo "FAIL: $1" >&2
	FAIL=1
}

search() {
	# usage: search PATTERN PATH...
	local pat="$1"
	shift
	if command -v rg >/dev/null 2>&1; then
		rg -n --glob '!**/third_party/**' --glob '!**/build/**' \
			--glob '!**/check-invariants.*' -e "$pat" "$@" 2>/dev/null || true
	else
		grep -RIn --exclude-dir=third_party --exclude-dir=build \
			--exclude='check-invariants.*' -E "$pat" "$@" 2>/dev/null || true
	fi
}

search_i() {
	local pat="$1"
	shift
	if command -v rg >/dev/null 2>&1; then
		rg -n -i --glob '!**/third_party/**' --glob '!**/build/**' \
			--glob '!**/check-invariants.*' -e "$pat" "$@" 2>/dev/null || true
	else
		grep -RIn --exclude-dir=third_party --exclude-dir=build \
			--exclude='check-invariants.*' -Ei "$pat" "$@" 2>/dev/null || true
	fi
}

# --- 8. No ImGui in src/ -------------------------------------------------------
while IFS= read -r -d '' f; do
	if grep -nE '^[#[:space:]]*include[[:space:]]*[<"][^>"]*imgui' "$f" >/dev/null 2>&1; then
		hit "ImGui include in src/ ($f) — UI via IMui / packs only"
		grep -nE '^[#[:space:]]*include[[:space:]]*[<"][^>"]*imgui' "$f" >&2 || true
	fi
	if grep -nE '\bImGui::' "$f" >/dev/null 2>&1; then
		hit "ImGui:: call in src/ ($f) — UI via IMui / packs only"
		grep -nE '\bImGui::' "$f" >&2 || true
	fi
done < <(find src -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
	! -path '*/build/*' -print0 2>/dev/null)

# --- 4. rigComponent = data (no systems-shaped sources) ------------------------
if [[ -d packs/rigComponent/src ]]; then
	while IFS= read -r -d '' f; do
		base="$(basename "$f")"
		case "$base" in
			S*.cpp|S*.h|*System*.cpp|*System*.h|*Systems*.cpp|*Systems*.h)
				hit "systems-shaped file in rigComponent ($f) — data pack only"
				;;
		esac
	done < <(find packs/rigComponent/src -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
		! -path '*/third_party/*' -print0 2>/dev/null)
fi

# --- 6. One vocabulary (positive misuse only; "never addon" teaching is ok) ----
VOCAB_PATHS=(AGENTS.md docs .cursor/rules .cursor/skills skills src examples tools cmake templates)
for pat in 'addons/' 'checkout_addon' '\bIAddon\b' '\bMAddon\b' '\bAddonRegistry\b'; do
	matches="$(search "$pat" "${VOCAB_PATHS[@]}")"
	if [[ -n "$matches" ]]; then
		hit "banned vocabulary /$pat/ (use pack / packs/)"
		echo "$matches" >&2
	fi
done

# --- 10. Next-year: archaeology in living first-party sources -------------------
matches="$(search_i '\bformerly\b|day-one|day one' src examples)"
if [[ -n "$matches" ]]; then
	hit "archaeology phrasing in src/ or examples/ (next-year docs only)"
	echo "$matches" >&2
fi

# Living prose (skip commandments tablet + skills that ban the phrases)
for f in docs/authoring.md docs/contributing.md docs/packs_using.md \
	docs/contract/README.md docs/contract/sude-loop.md docs/contract/rigkit.md \
	docs/contract/ui.md docs/contract/distribution.md docs/contract/pi-host.md; do
	[[ -f "$f" ]] || continue
	matches="$(search_i '\bformerly\b|day-one|day one' "$f")"
	if [[ -n "$matches" ]]; then
		hit "archaeology phrasing in $f (next-year docs only)"
		echo "$matches" >&2
	fi
done

# --- 10. No fake stubs (published product path that admits it does not work) ----
STUB_PATHS=(src examples packs)
# Skip tablet / skills that name the ban; skip third_party via search().
matches="$(search_i 'not implemented yet|rigGCode scaffold|\(scaffold\)' "${STUB_PATHS[@]}")"
# Allow the commandments tablet and deslop skill to discuss the rule.
if [[ -n "$matches" ]]; then
	filtered="$(echo "$matches" | grep -vE 'docs/contract/commandments\.md|rigkit-deslop|check-invariants' || true)"
	if [[ -n "$filtered" ]]; then
		hit "fake-stub phrasing in first-party code/docs (finish or do not publish)"
		echo "$filtered" >&2
	fi
fi

# --- Manifest license fields (SPDX in pack.json / app.json) --------------------
# Every checked-out pack, scaffold, and app manifest must declare license so
# GPL/MIT mixups are visible without opening LICENSE files.
check_manifest_license() {
	local mj="$1"
	local kind="$2"
	[[ -f "$mj" ]] || return 0
	if ! grep -qE '"license"[[:space:]]*:[[:space:]]*"[^"]+"' "$mj"; then
		hit "missing $kind license field ($mj) — SPDX string required"
		return
	fi
	# SPDX id, optional " Rigkid Contributors" (matches LICENSE copyright holder).
	if ! grep -qE '"license"[[:space:]]*:[[:space:]]*"(MIT|GPL-2\.0-or-later)( Rigkid Contributors)?"' "$mj"; then
		hit "unknown $kind license in $mj (expected MIT or GPL-2.0-or-later[, Rigkid Contributors])"
	fi
}
if [[ -d packs ]]; then
	for pj in packs/*/pack.json; do
		[[ -f "$pj" ]] || continue
		check_manifest_license "$pj" "pack.json"
	done
fi
check_manifest_license "templates/rigTemplate/pack.json" "pack.json"
# GPL pack must not claim MIT in the machine field.
if [[ -f packs/rigPlotFinders/pack.json ]]; then
	if ! grep -qE '"license"[[:space:]]*:[[:space:]]*"GPL-2\.0-or-later( Rigkid Contributors)?"' packs/rigPlotFinders/pack.json; then
		hit "rigPlotFinders must declare GPL-2.0-or-later (vendors Potrace)"
	fi
fi
# app.json — host examples, templates, contract smoke, and pack hero apps.
# Cap depth so example build/ trees are not walked.
while IFS= read -r -d '' aj; do
	check_manifest_license "$aj" "app.json"
done < <(
	find examples -maxdepth 2 -name app.json -print0 2>/dev/null
	find templates/app -maxdepth 1 -name app.json -print0 2>/dev/null
	find templates/rigTemplate/examples -maxdepth 2 -name app.json -print0 2>/dev/null
	find tools/contract_smoke -maxdepth 1 -name app.json -print0 2>/dev/null
	find packs/*/examples -maxdepth 2 -name app.json -print0 2>/dev/null
)
if [[ -f packs/rigPlotFinders/examples/finders/app.json ]]; then
	if ! grep -qE '"license"[[:space:]]*:[[:space:]]*"GPL-2\.0-or-later"' \
		packs/rigPlotFinders/examples/finders/app.json; then
		hit "rigPlotFinders hero app.json must declare GPL-2.0-or-later"
	fi
fi

# --- Pack constructors must not re-author pack.json identity --------------------
# description / license / url / dependencies live in pack.json; MPack applies them.
# Flag ctor-era setters so scaffolds and local packs cannot reintroduce a second copy.
check_pack_ctor_identity() {
	local dir="$1"
	[[ -d "$dir" ]] || return 0
	local hits
	hits="$(grep -RInE --include='*.cpp' \
		'setDescription\s*\(|setLicense\s*\(|setUrl\s*\(|addDependency\s*\(' \
		"$dir" 2>/dev/null | grep -vE '/(build|third_party|\.git|examples)/' || true)"
	if [[ -n "$hits" ]]; then
		hit "pack ctor must not set identity/deps (use pack.json): $dir"
		echo "$hits" >&2
	fi
}
check_pack_ctor_identity "templates/rigTemplate/src"
if [[ -d packs ]]; then
	for pj in packs/*/pack.json; do
		[[ -f "$pj" ]] || continue
		check_pack_ctor_identity "$(dirname "$pj")/src"
	done
fi

# --- Every pack: hero example + README screenshot ------------------------------
# packs/<name>/examples/<hero>/CMakeLists.txt and examples/<hero>/img/preview.png
# Pack README must embed preview.png. Host examples/ do not count.
if [[ -d packs ]]; then
	for pj in packs/*/pack.json; do
		[[ -f "$pj" ]] || continue
		pack_dir="$(dirname "$pj")"
		pack_name="$(basename "$pack_dir")"
		hero_ok=
		while IFS= read -r -d '' hero_cmake; do
			hero_dir="$(dirname "$hero_cmake")"
			if [[ -f "$hero_dir/img/preview.png" ]]; then
				hero_ok=1
				break
			fi
		done < <(find "$pack_dir/examples" -mindepth 2 -maxdepth 2 -name CMakeLists.txt -print0 2>/dev/null)
		if [[ -z "$hero_ok" ]]; then
			hit "pack $pack_name needs examples/<hero>/ with CMakeLists.txt and img/preview.png"
		fi
		readme="$pack_dir/README.md"
		if [[ -f "$readme" ]] && ! grep -q 'preview\.png' "$readme"; then
			hit "pack $pack_name README.md must embed examples/<hero>/img/preview.png"
		fi
	done
fi

if [[ "$FAIL" -ne 0 ]]; then
	echo "" >&2
	echo "Ten Commandments invariant check failed. See docs/contract/commandments.md" >&2
	exit 1
fi

echo "OK: check-invariants (Ten Commandments machine gates)"
