---
name: rigkit-comments
description: >-
  Write and revise RigKit comments and first-party docs in plain why-and-how,
  no fluff. Public APIs use Doxygen-compatible tags (@brief, @param, @return,
  …). Use when adding header docs, editing contract/authoring Markdown,
  explaining present paths or pack boundaries, or cleaning prose.
---

# RigKit comments (Weissflog voice + Doxygen public API)

Comments and first-party Markdown (`docs/`, `AGENTS.md`, pack READMEs) share the same voice — plain English, usage first, clear about what something does **not** do.

**Public APIs** additionally use tags compatible with Doxygen (and similar generators). Private / local notes stay plain `//`.

Generate HTML from those tags: `cmake --build build --target docs` writes `build/docs/api/html/index.html` (see [docs/api/](../../docs/api/), [rigkit-build](../rigkit-build/SKILL.md)).

Also: no emoji in comments or UI text ([AGENTS.md](../../AGENTS.md)). No unicode arrows in Markdown — write “to”, “then”, or “>” for menus (not U+2192 / U+2194 / U+21D2). `tools/check-invariants` gates the glyphs.

## Voice

- **Next-year rule.** Write as if someone reads this next year. Never “day-one”, “from day one”, “OOTB on day one”, or onboarding-flavored urgency. Say what ships now: default build, in-org pack, distribution.
- **Why and how to use**, not a narration of the next line (`// increment i`).
- **Plain prose.** Short sentences. Prefer “does not” lists over marketing claims.
- **Clear limits.** Stubs, Pi/GLES constraints, “Blend2D is optional pack” — say it.
- **No “honest”.** Ban the word: not “honest subset”, “an honest…”, “honesty model”, “Weissflog-honest”. Say the fact (“ESP32 subset”, “same model as third_party”) — don’t soft-sell it.
- **Put docs at the decision.** File/header block for module role; one line on non-obvious call sites.
- **No AI sludge:** “This function ensures that…”, “In order to…”, “Leverages…”, restating types already in the signature.
- **No ceremony:** no banners of `====`, no author/date stamps, no commented-out code as history.
- **No archaeology:** never “formerly…”, “used to be…”, “backwards compatible with…”, “BeeB”, “Stack” (product name), or rename breadcrumbs. Describe what it **is** now. Migration notes belong in a commit / changelog, not the living docs.
- **No unicode arrows in Markdown.** Write “to”, “then”, or “>” for menus. Not U+2192 / U+2194 / U+21D2.

## Terms (keep straight)

| Say | Mean |
|-----|------|
| **RigWorks** / **Rig** | Zero-code creative framework (SUDE + ECS + shared POD schemas). Product name RigWorks; **Rig** in running text. [honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md). Not a library. |
| **Contract / framework** | Rig rules. Not RigKit the product. |
| **Fulfillment** | Concrete host/pack implementing rules (`rigImGui`, POD table, …) |
| **RigKit** | This coded host — **is Rig**; author path is **Rig + UI** |
| **Host** | Runtime pillar (`RigKitEngine`, managers, `src/`) |
| **SUDE** / **ECS** | Required layers to be Rig |
| **UI** | Optional companion (**Rig + UI**); not required to be Rig |
| **Schema** | Agreed POD field layout — ship what you support ([schemas](https://github.com/rigkid/RigWorks/tree/main/schemas)) |
| **SUDE loop** | Setup / Update / Draw / Exit ([sude-loop.md](../../docs/contract/sude-loop.md)) |
| **Canvas** | Render surface / FBO type (`MCanvas`) — not the host pillar |
| **pack** | Capability unit under `packs/` (`pack.json`, `IPack`). Never “addon”. Spoken name = folder = `pack.json` `"name"` = `app.json` `"name"`. |
| **In-org packs** | First-party rigkid org packs under `packs/` |

## Public API to Doxygen tags

Document types and functions that apps / packs call across a boundary (`IApp`, `IMui`, `MEcs` registration, `Canvas`, `OpenGLRenderer` / `IRenderer`, author `rig::` helpers, pack entry headers).

Prefer `/** … */` or `///` with `@` commands (Javadoc-style; Doxygen accepts both).

| Tag | Use |
|-----|-----|
| `@brief` | One-line summary (required on public types/functions) |
| `@details` | Optional longer why / does-not / usage (Weissflog prose here) |
| `@param name` | Non-obvious parameters only; skip self-explanatory names when the brief is enough |
| `@return` | When the return value carries meaning beyond the type |
| `@note` / `@warning` | Pi, re-entrancy, ownership, pack bootstrap order |
| `@deprecated` | With replacement |
| `@see` | Related type, doc, or pack |

Do **not** Doxygen-decorate private helpers, `.cpp`-local statics, or obvious POD field getters. Keep POD component headers light — a short `@brief` on the struct is enough; fields usually speak for themselves.

### Public type / function (good)

```cpp
/**
 * @brief Default host Draw fulfillment — immediate GL primitives.
 * @details Draws to the current framebuffer (typically the window).
 * Does not create a window or swap buffers. Matches SShapeRendering call
 * style: setFillColor then draw* fills; setStrokeColor then draw* strokes.
 * @see IRenderer, MEcs::setPresentRenderer
 */
class OpenGLRenderer : public IRenderer {
```

```cpp
/**
 * @brief Create a rectangle entity with transform, shape, and draw style.
 * @param ecs Target registry wrapper.
 * @param x Top-left x in canvas space.
 * @param y Top-left y in canvas space.
 * @param w Width.
 * @param h Height.
 * @param style Fill/stroke POD (default white fill).
 * @param name Optional entity name for lookup.
 * @return New entity id.
 */
entt::entity makeRect(rigkit::MEcs& ecs, float x, float y, float w, float h,
					  const rigkit::ecs::CDrawStyle& style = fill(1, 1, 1),
					  const std::string& name = {});
```

## Doxygen traps that silently eat text

`tools/check-docs` fails the build on any warning, because a warning here means text is missing from the HTML rather than merely untidy. Some of these truncate **without** warning, so a clean log is not proof — see "Silence is not proof" below.

### A dot before punctuation ends the brief

The one trap behind most breakage: a `.` immediately followed by `*`, `<`, or a closing backtick ends the brief description right there. Everything after it is cut off the brief. When that spot sits inside a code span the span is split across the boundary, which is the `end of comment block while expecting </tt>` warning.

Escape the dot as `\.`, and keep it **outside** backticks — an escape inside a code span is not processed. `` `rig\.*` `` still breaks; `@c rig\.*` renders `rig.*` and is clean.

| Write this | Not this |
|------------|----------|
| `@c rig\.*` or bare `rig\.*` | `` `rig.*` ``, `@c rig.*`, bare `rig.*` |
| `@c _rigkit\.` | `` `_rigkit.` `` |
| bare `x\.\<vendor\>\.\<name\>` | `` `x.<vendor>.<name>` `` |

Only a dot before punctuation bites. `` `x*2` ``, `` `make*` ``, `` `countU * countV` ``, and `` `x.rigkit.net_scan` `` are all fine.

### Angle brackets

Backticks are enough on their own: `` `tools/<name>.json` ``, `` `<exeDir>/data` ``, and `` `std::vector<float>` `` all render whole. Left bare, an angle bracket warns as an unsupported HTML tag and the tag itself is dropped, though the surrounding sentence survives. Backticks stop helping once a dot precedes the bracket — then escape the whole thing bare, as in the table above.

### `@ref` needs a target Doxygen actually emits

| Target | Form that resolves |
|--------|--------------------|
| Namespace-scope free function | Fully qualified: `@ref rigkit::plot::climbRoundedPoints` |
| Enum at namespace scope | Fully qualified, and the file needs `@file` |
| Undocumented member, `constexpr`, file-static | None — use backticks: `` `kTitle` `` |

For a free function inside a namespace, the bare name, a trailing `()`, and a full signature all fail; only the qualified name works. A `:` straight after a qualified name is absorbed into it, so follow the reference with an em dash instead of a colon.

`EXTRACT_ALL` and `EXTRACT_STATIC` are both `NO`, so a one-line accessor with no doc comment is not in the output and nothing can link to it. Document the target or use backticks.

### `@file` or the free functions vanish

**A header with no `@file` block has its free functions and namespace-scope enums dropped entirely.** Doxygen documents file-scope members only when the file itself is documented; structs and classes appear either way. Any header declaring namespace-scope functions or enums needs:

```cpp
/**
 * @file
 * @brief Run prepare-pipeline steps and per-layer path effects over `CPaths`.
 */
```

### Silence is not proof

Only the span-splitting case warns. A dot before punctuation outside a code span truncates the brief with an empty warning log, so `check-docs` passing does not mean a given sentence survived. When editing a brief that contains a wildcard or a placeholder, read the generated `annotated.html` and confirm the end of the sentence is there.

### Local note (good — not Doxygen)

```cpp
// Already inside Draw — call runSystemByName, never renderSystems() again.
```

### Bad

```cpp
// This function renders the shape by calling the renderer draw methods.
void renderRectangle(...);
```

## Where comments earn their keep

| Place | What to say |
|-------|-------------|
| Pack / module header | `@brief` role; `@details` bootstrap order / what stays out |
| Present / SUDE | Who sets `IRenderer`, when Draw runs, Canvas FBO vs window |
| Non-obvious invariants | Why raw `IRenderer*`, why re-entrancy guard, why data vs fulfillment |
| Public author API | Doxygen + friendly usage examples |
| Deliberate absences | `@note` Drawing preview lives in a tool pack — not here |

## Pass checklist

When asked to improve comments or docs:

1. Delete narrating / duplicate / outdated comments — including formerly / day-one / “honest…” / backwards-compat breadcrumbs.
2. Public APIs: ensure `@brief` (and `@param` / `@return` / `@note` where useful).
3. Private code: plain `//` why-notes only.
4. Keep tabs / clang-format; do not reflow entire files for prose alone.
5. Match surrounding file tone — denser on host/present APIs, lighter on tiny POD headers.
6. Markdown: present tense; next-year rule; never rename history; no unicode arrows.

## Output

Edit comments in place. Summarize in a few bullets: Doxygen coverage added, sludge removed, files touched.
