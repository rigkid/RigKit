# rig-validate

Validate Rig JSON documents against [`schemas/json/`](../../schemas/json/).

```bash
npm run setup                                          # from the repo root
node tools/rig-validate/cli.js examples/minimal-scene.json
node tools/rig-validate/cli.js --strict path/to/doc.json
```

## What it checks

| Check | Level |
|-------|-------|
| Document envelope against [`rig.document`](../../schemas/document.md) | error |
| Each component against its schema id | error |
| Duplicate entity ids | error |
| `entity`-typed fields pointing at ids that do not exist (`null` means none) | error |
| Unknown schema ids | warn, or error with `--strict` |
| Document targets a version newer than [`VERSION`](../../VERSION) | warn |

Failures report a JSON Pointer path. Exit `1` if any error; warnings alone exit `0`.

## Repository checks

```bash
npm run check          # all of the below
npm run check:schemas  # schemas/json/ matches tools/gen-schemas.mjs
npm run check:parity   # prose schema ids <-> JSON Schema files
npm run check:links    # relative links in all markdown resolve
npm run check:examples # goldens validate in --strict mode
```

Regenerate the machine grammar after editing the catalog in [`tools/gen-schemas.mjs`](../gen-schemas.mjs):

```bash
npm run gen
```
