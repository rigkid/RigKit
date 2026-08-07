# AI co-coding

**AI co-coding is here to stay.** Not as a prediction to argue about — as a design constraint we address.

## What changes?

If a model can write a renderer, a parser, or an inspector in an afternoon, then implementations stop being the scarce thing. What stays scarce is **agreement**: what a rectangle is, which units a transform uses, whether rotation is a quaternion or three angles.

That inverts the usual priority. A generated renderer is fine and disposable. A generated *vocabulary* is a liability, because the next session invents a different one. [Concept over execution](why-no-code.md) is not a slogan here; it is the only part a model cannot usefully improvise for you.

## Why a vocabulary beats a prompt

| | A prompt | A schema id |
|--|----------|-------------|
| Output | Something plausible | `rig.spatial.transform`, spelled the same way |
| Across sessions | Drifts | Stable |
| Across models and vendors | Drifts | Stable |
| Across two different apps | Incompatible | Interoperable |
| Verifiable | By reading it | By running a validator |

Two agents working in two unrelated applications converge on compatible data — not because they coordinated, but because they read the same catalog. That convergence is the product. And makes coding together more fun.

## Not another editor

Rig is **not** a node editor, a patcher, or an agentic design canvas. Those are fulfillments — including excellent ones that still bottom out in code (TouchDesigner, Max, vvvv). Rig is the vocabulary editors and agents could share so their content interoperates without anyone linking a library.

## What Rig hands a model

A no-code framework is, conveniently, the exact shape a model works best with: no API to guess, no build to fight, only data against a published grammar.

| Asset | Role |
|-------|------|
| [`llms.txt`](../llms.txt) | Discovery — the entry point for generic tooling |
| [`skills/generating-rig-documents/SKILL.md`](../skills/generating-rig-documents/SKILL.md) | The condensed rules, sized to load before generating |
| [`AGENTS.md`](../AGENTS.md) | Cross-agent onboarding, pointing at the skill and the validator |
| [`schemas/`](../schemas/) | Prose — meaning and units, for the reasoning step |
| [`schemas/json/`](../schemas/json/) | Machine grammar — the checkable truth |
| [`examples/`](../examples/) | Golden documents to copy instead of invent |
| [`tools/rig-validate/`](../tools/rig-validate/) | The verdict: JSON Pointer paths and an exit code |

Two documents deliberately say the same things at different densities. Prose is for understanding, the skill is for working memory, JSON Schema is for enforcement.

## The closed loop

Generation without verification is a guess. The loop is what makes AI-authored Rig trustworthy:

```text
read the skill → copy a golden example → emit JSON → validate → fix at the reported path → repeat until ok
```

```bash
node tools/rig-validate/cli.js path/to/doc.json
```

Every failure names a JSON Pointer, so a model can correct itself without a human interpreting the error. `--strict` turns invented schema ids into failures, which is the check worth running when you suspect confident guessing.

Never deliver an unvalidated document. Eyeballing JSON is exactly the review that looks fine and is wrong.

## Working with agents here

| Do | Why |
|----|-----|
| Point the agent at the skill, not the whole repo | A short skill beats the whole tree; the skill exists for this |
| Run `--strict` on unfamiliar output | Invented ids are the most common confident error |
| Prefer additive schema changes | Documents, prompts, and model habits in the wild all age better |
| Keep prose and JSON Schema in parity | Models read prose, tools read grammar — drift teaches the model something false |
| Spend review time on concepts | New ids, units, and portable-vs-[host-cache](terms.md) calls are yours; field-by-field diffs are not |

Parity is enforced, not trusted:

```bash
node tools/check-schema-parity/check.mjs
```

## Why "co-"

The division of labour is the same one the Contract already draws. **You own the concept** — which ids exist, what they mean, what units they carry, what stays portable. **The model is very good at execution** — emitting documents, filling catalogs, writing the host that consumes them, porting a fulfillment to a new language.
