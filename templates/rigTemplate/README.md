# rigTemplate

Scaffold for a new **RigKit** pack (STATIC library + `pack.json`).

Seed remote: `https://github.com/rigkid/rigTemplate.git`  
Host copy lives at `templates/rigTemplate/` in the RigKit repo (not loaded as an app dependency).

## Create a new pack

**Before you scaffold:** read [packs/README.md](../../packs/README.md) (pack table + Naming). Prefer growing an existing pack over a new remote. Spoken name = folder = `pack.json` `"name"` = `app.json` `"name"` - one camelCase id.

1. Lock that id. Copy this tree to `packs/<id>/` (or clone the `rigTemplate` remote and rename).
2. Rename everywhere to the same id:
   - folder, `pack.json` `name` / `url` / `description` / `dependencies`
   - `src/rigTemplate.*` to `src/<id>.*`
   - class `rigTemplate` and `PackRegistry` factory string
   - `CMakeLists.txt` source list
   - README footer `[API/docs](https://rigkid.github.io/<id>/)`
   - Leave the constructor as `IPack("<id>")` only - identity and deps come from `pack.json`
3. Fill `setup()`: **data** to `registerComponent`; **code** to `registerSystem`. Keep portable fields POD (NO CODE JUST DATA).
4. Add to your app `app.json`:

```json
{
  "name": "yourId",
  "url": "https://github.com/rigkid/yourId.git",
  "ref": "main"
}
```

5. Bootstrap order: register data pack before systems packs before UI (`rigComponent`, then `rigSystems`, then `rigImGui`).

## Style

- `.clang-format` / `.editorconfig` match RigKit (tabs, C++20, `SortIncludes`).
- Format first-party `src/` + `examples/`: `./tools/format.sh` or `tools\format.bat`. Never `third_party/`.
- Pre-commit (once per clone): `./tools/install-hooks.sh` or `tools\install-hooks.bat`. From a RigKit host checkout, `./tools/install-hooks.sh` also wires every `packs/<name>` git repo.
- Public headers: Doxygen `@brief` ([rigkit-comments](../../skills/rigkit-comments/SKILL.md)).

## CI

Code packs keep `examples/demo`. After renaming this scaffold, set `PACK` / `EXAMPLE` in `.github/workflows/ci.yml` and capture `img/preview.png` when the window is the README picture. Data packs (`*Component`) delete `examples/` and compile the static library in that workflow instead. They do not need a demo or a screenshot.

`.github/workflows/docs.yml` publishes pack API docs to `https://rigkid.github.io/<pack>/` (host reusable `pack-docs.yml`). Rename the `pack:` input to your id. Enable Pages (GitHub Actions) once on the pack remote after host `pack-docs.yml` is on `main`.

## Publish

From the RigKit host:

```bash
./tools/publish-template.sh
```

Or treat like any pack remote and push this tree to `rigkid/rigTemplate`.

[API/docs](https://rigkid.github.io/rigTemplate/)
