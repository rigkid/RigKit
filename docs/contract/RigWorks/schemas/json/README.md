# JSON Schema

Machine-readable Rig POD and document grammar (JSON Schema draft 2020-12).

| File | Role |
|------|------|
| `_defs.schema.json` | Shared datatypes (`vec3`, `quat`, `entity`, …) |
| `rig.document.schema.json` | Document envelope |
| `rig.*.schema.json` | One file per catalog schema id |

Regenerate component + document schemas from the catalog script:

```bash
node tools/gen-schemas.mjs
```

Validate documents:

```bash
cd tools/rig-validate && npm install && cd ../..
node tools/rig-validate/cli.js examples/minimal-scene.json
```

Prose field meaning stays in the sibling markdown under [`../`](../). Keep them in parity:

```bash
node tools/check-schema-parity/check.mjs
```
