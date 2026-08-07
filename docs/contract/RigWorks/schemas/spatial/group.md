# `rig.spatial.group`

Scene group marker. Format when present — presence means this entity is a group root (folder), not a drawable by itself.

No fields. Compose:

- [`rig.spatial.relationship`](relationship.md) — children set `parent` to this entity
- [`rig.spatial.transform`](transform.md) — group pose (usual)
- [`rig.meta.named`](../meta/named.md) — label
- optionally [`rig.spatial.layer`](layer.md) — list chrome

Do **not** add a second parent field. Do **not** reuse [`rig.pixel.layer`](../pixel/layer.md) `groupParent` (compositor graph). Node editor groups are [`rig.node.node`](../node/node.md) with `nested`, not this schema.
