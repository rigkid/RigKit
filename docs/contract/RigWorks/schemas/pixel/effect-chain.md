# `rig.pixel.effect_chain`

Ordered effect steps. Format when present.

## Step

| Field | Type | Meaning |
|-------|------|---------|
| `id` | uint | Stable id within the chain |
| `stage` | enum | image, draw, generate |
| `effectId` | string | Registry name |
| `enabled` | bool | Optional. Skip when false; absent = true |
| `parentStep` | uint | Optional. Input step id; absent = the preceding step |

## Chain

| Field | Type | Meaning |
|-------|------|---------|
| `steps` | step[] | Ordered list |
| `nextId` | uint | Optional. Next id allocator for steps; absent = highest step id + 1 |

Effect-specific parameters are **host / pack extension** (side table or JSON text keyed by step). No GPU programs in the schema.

## Input routing

`parentStep` names the step whose result this step consumes. It is dataflow, not containment: several steps may share one `parentStep` and each reads that same result, which is how a chain branches.

With `parentStep` unset every step reads the one before it, so a plain ordered list needs no routing fields at all. Editors that draw the chain as a tree indent a step under its `parentStep`.

`parentStep` is **not** `groupParent` on [`rig.pixel.layer`](layer.md) — the compositor stack is a different graph.
