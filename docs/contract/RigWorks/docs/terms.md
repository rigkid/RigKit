# Terms

Rig prose leans on a few words in a precise way. This page is the reference.

## Contract and fulfillment

| Term | Meaning |
|------|---------|
| **Contract** | The concept. Rules ([SUDE](sude.md) + [ECS](ecs.md)), schema ids, field names, units, [datatypes](properties.md). Portable. Versioned. Written down, never compiled. |
| **Fulfillment** | The execution. Any code that honors the Contract — a host, a pack, a renderer, a UI. Replaceable. |

Everything in this repository is Contract. Nothing in this repository is fulfillment.

When a document says a detail is "fulfillment, not Contract", it means: implement it however you like, and do not expect other hosts to match you. When it says "Contract-facing", it means the field or seam is portable, so keep host types out of it.

## Roles

| Term | Meaning |
|------|---------|
| **Host** | The program that runs the SUDE loop and owns the registry, clock, and windowing. [RigKit](https://github.com/rigkid/RigKit) is the reference host. |
| **Pack** | An optional module a host loads to add capability — a UI pack, a device pack, a render backend. Packs are fulfillment; they interoperate through shared schema ids, not through a shared API. |
| **App** | The piece being authored. Owns its entities. Runs inside a host. |
| **Document** | A [`rig.document`](../schemas/document.md) JSON payload — entities and their components, on the wire. |

## Data words

| Term | Meaning |
|------|---------|
| **POD** | Plain old data. Numbers, small structs, strings, entity ids. No pointers, handles, callbacks, or toolkit types. |
| **Entity** | An id that owns a set of components. Carries no behavior. |
| **Component** | A named POD payload attached to an entity, keyed by schema id (`rig.spatial.transform`). |
| **Schema id** | The stable name of a component layout: `rig.<domain>.<name>`. The unit of interoperability — two hosts that speak the same id share fields and units. |
| **Portable** | Meaning survives leaving your process. Portable fields get serialized; everything else stays in the host. |
| **Host cache** | A value a host derives and may keep, but must not serialize — world matrices, Euler angles, an LFO's last sample, hover state. |

## Reading the rules

"Is it Rig?" is answered in [honors.md](honors.md): SUDE plus ECS, nothing more. Schemas are opt-in — [ship what you support](../schemas/README.md). UI is a separate, optional layer ([ui.md](ui.md)).
