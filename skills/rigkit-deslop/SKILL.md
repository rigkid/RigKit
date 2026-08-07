---
name: rigkit-deslop
description: >-
  Clean AI-generated or migration slop in RigKit without redesign. Use when the
  user says deslop, anti-slop, AI slop, state churn, or asks to clean noisy
  wrappers, dead code, leftover stubs, visibility maps, hideWindow title lists,
  m_last* mirrors, per-frame syncFromPrefs, save/restore bookkeeping, or a flag
  whose default is duplicated across layers. Also for write-only state (a field
  written but never read), setter pairs reconciled by a hidden intent/mode flag,
  and retained state that makes call order load-bearing.
  Complements rigkit-minimal (prevent) with a deletion-first cleanup workflow
  (repair).
---

# RigKit deslop

Cleanup pass for code that works but is bloated, duplicated, or fake. Not for new features or broad redesigns.

Capability units are **packs** — never reintroduce “addon” wording. Keep one pack id (spoken name = folder = manifests); do not leave mismatched display titles or rename synonyms.

Pairs with [rigkit-minimal](../rigkit-minimal/SKILL.md): minimal prevents slop; deslop removes what already landed.

## When

- User says `deslop`, `anti-slop`, `state churn`, or “clean the AI noise”
- Follow-up left dead helpers, fake `shared_ptr`, flag unused shaders, empty TODOs
- Code saves/restores state, mirrors prefs, or scrubs windows by title
- Reviewer-only: suggest first; edit only when asked to apply

## When not

- New feature / product change
- “Refactor for SSO” style redesigns
- Formatting-only (use `tools/format.*`)

## RigKit filter (finish vs delete)

| Kind | Action |
|------|--------|
| Superseded by present/registry path | **Delete** |
| Real framework capability, empty today | **Finish** or park in the right layer — never leave a fake no-op in the wrong place (Commandment 10) |
| Setting / property row nothing reads | **Finish** (wire the reader) or delete the row — never leave an editable control with no effect |

When the row is ambiguous — a stub could be a wanted capability, not just leftovers — **ask the user delete-or-finish before editing**. One question per item with path + evidence + what "finish" would mean. Only clear cases (no callers (even out of tree?), no product value, superseded) are deleted without asking.

Examples of delete: legacy `MEcs` runners replaced by `registerSystem`; dead `Graphics` GL that never draws; unused converters.

Examples of finish: selection overlay over `CSelection`; Canvas FBO present; OpenGL `switchRenderer`. Drawing preview → tool-session data in a tool pack (see TODO / authoring.md), not a core stub.

## Workflow

1. **Lock behavior** — name what must stay. Prefer smoke-build / run `minimal` or the touched example when unit tests are thin.
2. **Plan before edit** — bound files; list smells; safest deletes first.
3. **Classify** — duplication · dead code · needless abstraction · **state churn** · **pass-back args** · **set-then-use setters** · **write-only state** · **dead control** · boundary leak (ImGui in `src/`, systems in `rigComponent`) · docs overclaim · missing verify.
4. **One smell pass at a time** — delete → consolidate → naming → verify. Re-build after each pass.
5. **Report** — changed files · simplifications · verification · remaining risks.

## State churn smell

Mutation bookkeeping where a read-time gate works. Delete the bookkeeping, keep the gate — see [rigkit-minimal](../rigkit-minimal/SKILL.md#state-churn-gate-dont-mutate).

| Churn | Replace with |
|-------|--------------|
| Save visibility map → hide all → restore | Skip rendering behind a bool gate |
| `hideWindow("Log")` title lists after create | Declare visibility at create, or don’t create it |
| N `m_last*` shadow fields for a rebuild check | One epoch / dirty counter |
| `syncFromPrefs()` every frame into mirrors | Read prefs where they are used |
| Branching on display text (`left() == "Edit Mode"`) | Branch on the bool; render text from it |
| `*Applied` / `have*Pending` sentinel pairs | One value applied once at the real hook |
| Same default literal in engine + impl + interface | One owner holds it; the rest read through |
| Setter on an interface that every impl copies | Reader on the dependent side (pull, don't push) |
| Argument the callee already reaches (manager handed back, stored `m_*` passed in, second pointer to one owner) | Drop the parameter; read through the owner — [no pass-back](../rigkit-minimal/SKILL.md#no-pass-back-never-hand-over-what-the-callee-already-reaches) |
| Lambda whose only job is forwarding args to one call | Register / call the function directly |
| `setX()` pairs a hidden mode / intent flag reconciles (`setFillColor` + `setStrokeColor` → `m_intent`) | Pass the value with the call as a POD — [no set-then-use](../rigkit-minimal/SKILL.md#no-set-then-use-per-call-data-is-a-parameter-not-stored-state) |
| Member assigned but never read (`m_hasFill`, `m_fillOpacity`) | Delete it, or wire the reader — a setter with no reader is a dead control |

**Finding write-only state:** grep each suspect member — if every hit is an assignment, nothing reads it. Cheap, and it catches the whole class; no compiler flag does, since an assignment counts as a use and `-Wunused-private-field` stays quiet.

Keep genuine deferrals (ImGui font atlas rebuild, GLFW drop callback → next frame, GL state save/restore around foreign draws).

## Posture

- Preserve behavior unless asked otherwise.
- Prefer deletion over addition; reuse before new helpers.
- No new dependencies.
- Stay on the requested file list; do not widen into “while we’re here.”
- Respect pack split, Pi floor, no Dear ImGui in `src/` ([AGENTS.md](../../AGENTS.md)).

## Review-only mode

If the user asks for suggestions only (`--review` / “what would you deslop”): do not edit. Produce ranked findings with path + evidence + delete-or-finish + risk.
