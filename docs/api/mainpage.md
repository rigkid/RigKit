# RigKit API reference {#mainpage}

Generated from Doxygen tags on public headers (`@brief`, `@param`, `@return`, …).

Published host aggregate: [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/).  
Site landing: [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/).  
Per-pack sites: `https://rigkid.github.io/<packName>/` (same Doxygen tags, pack headers only).

This is **not** the Contract or authoring guide — those stay in Markdown in the repo:

| Doc | Role |
|-----|------|
| `docs/authoring.md` | How artists code RigKit |
| `docs/contract/` | SUDE loop, Host, Pi, packs |
| `AGENTS.md` | Agent / contributor playbook |

## What is indexed

- **Core host:** `src/` — engine, `IApp`, Canvas, ECS registry, present / `IRenderer`
- **Modules (packs):** first-party headers under `packs/*/src` (and other pack `.h` trees). Vendored `third_party/` trees (Dear ImGui, etc.) are excluded.
- New packs dropped under `packs/<name>/` are picked up automatically once they have documented headers. For CI aggregate, also list the remote in [`pack-remotes.txt`](pack-remotes.txt).

Undocumented private helpers are hidden. Prefer reading the narrative docs first, then use this reference for signatures and ownership notes.

## Generate

Requires [Doxygen](https://www.doxygen.nl/) on `PATH`. Optional Graphviz (`dot`) for inheritance graphs.

```bash
cmake -S . -B build
cmake --build build --target docs
# open build/docs/api/html/index.html

# One pack only:
./tools/generate-pack-docs.sh rigComponent
```
