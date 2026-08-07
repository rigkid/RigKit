# Is it Rig?

Rig is a shared vocabulary: **[SUDE](sude.md) + [ECS](ecs.md)**, plus optional shared **[schemas](../schemas/)** (prose + [JSON Schema](../schemas/json/)). It is not a library to link into your app.

A project **is Rig** when:

1. It honors the [SUDE loop](sude.md).
2. It honors [ECS conventions](ecs.md).

Both matter — a SUDE loop by itself is a loop convention, not a shared vocabulary. **No ECS, no Rig.**

Schemas and UI are optional. Ship what you support. Portable documents use the [`rig.document`](../schemas/document.md) JSON envelope; [`rig-validate`](../tools/rig-validate/) checks emission against the machine grammar.

## What you may omit

Any host, any [UI](ui.md), any pack, device drivers, render backends, and every schema you do not implement.
You do not need a particular implementation to be Rig.

## Reference fulfillment

[RigKit](https://github.com/rigkid/RigKit) is the reference host: a desktop fulfillment (GLFW / OpenGL / packs) that runs the SUDE loop, owns an ECS registry, and reads and writes the schemas it supports. Read it as one worked answer, not as the definition — see [terms.md](terms.md) for what "fulfillment" claims and does not claim.

