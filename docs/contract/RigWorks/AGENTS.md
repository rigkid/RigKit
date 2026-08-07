# AGENTS

RigWorks (Rig for short) is a no-code creative application framework — a shared data vocabulary for creative applications (**not character rigging**). Apps and packs that speak the same schema ids interoperate. It is not a library to link and not another editor; hosts honor SUDE + ECS and emit/consume POD JSON.

## Before generating Rig data

1. Read [`skills/generating-rig-documents/SKILL.md`](skills/generating-rig-documents/SKILL.md).
2. Copy patterns from [`examples/`](examples/).
3. Validate every document you produce:

```bash
cd tools/rig-validate && npm install && cd ../..
node tools/rig-validate/cli.js path/to/doc.json
```

Do not deliver unvalidated output.

## Key paths

| Path | Role |
|------|------|
| [`skills/generating-rig-documents/SKILL.md`](skills/generating-rig-documents/SKILL.md) | Condensed AI-facing rules + examples |
| [`schemas/json/`](schemas/json/) | JSON Schema grammar |
| [`schemas/`](schemas/) | Prose field meaning |
| [`examples/`](examples/) | Golden documents |
| [`docs/interchange.md`](docs/interchange.md) | Wire format + RigKit key aliases |
| [`docs/terms.md`](docs/terms.md) | Contract vs fulfillment, host, pack, POD |
| [`tools/rig-validate/`](tools/rig-validate/) | Validator CLI |

Discovery index for generic LLM tooling: [`llms.txt`](llms.txt).
