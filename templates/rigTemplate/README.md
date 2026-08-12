# rigTemplate

Scaffold for a new **RigKit** pack (STATIC library + `pack.json`).

Seed remote: `https://github.com/rigkid/rigTemplate.git`  
Host copy lives at `templates/rigTemplate/` in the RigKit repo (not loaded as an app dependency).

## Create a new pack

**Before you scaffold:** read [packs/README.md](../../packs/README.md) (pack table + Naming). Prefer growing an existing pack over a new remote. Spoken name = folder = `pack.json` `"name"` = `app.json` `"name"` — one camelCase id.

1. Lock that id. Copy this tree to `packs/<id>/` (or clone the `rigTemplate` remote and rename).
2. Rename everywhere to the same id:
   - folder, `pack.json` `name` / `url` / `description` / `dependencies`
   - `src/rigTemplate.*` → `src/<id>.*`
   - class `rigTemplate` and `PackRegistry` factory string
   - `CMakeLists.txt` source list
   - README footer `[API/docs](https://rigkid.github.io/<id>/)`
   - Leave the constructor as `IPack("<id>")` only — identity and deps come from `pack.json`
3. Fill `setup()`: **data** → `registerComponent`; **code** → `registerSystem`. Keep portable fields POD (NO CODE JUST DATA).
4. Add to your app `app.json`:

```json
{
  "name": "yourId",
  "url": "https://github.com/rigkid/yourId.git",
  "ref": "main"
}
```

5. Bootstrap order: register data pack before systems packs before UI (`rigComponent` → `rigSystems` → `rigImGui`).

## Style

- `.clang-format` / `.editorconfig` match RigKit (tabs, C++20).
- Public headers: Doxygen `@brief` ([rigkit-comments](../../skills/rigkit-comments/SKILL.md)).

## CI

`.github/workflows/ci.yml` builds `examples/demo` against a RigKit host checkout. After renaming this scaffold, set `PACK` / `HERO` in that workflow to match your pack.

`.github/workflows/docs.yml` publishes pack API docs to `https://rigkid.github.io/<pack>/` (host reusable `pack-docs.yml`). Rename the `pack:` input to your id. Enable Pages (GitHub Actions) once on the pack remote after host `pack-docs.yml` is on `main`.

## Publish

From the RigKit host:

```bash
./tools/publish-template.sh
```

Or treat like any pack remote and push this tree to `rigkid/rigTemplate`.

[API/docs](https://rigkid.github.io/rigTemplate/)
