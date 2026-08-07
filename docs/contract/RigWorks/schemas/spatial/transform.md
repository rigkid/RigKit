# `rig.spatial.transform`

Local pose. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `position` | vec3 | Optional. Local translation; absent = (0, 0, 0) |
| `rotation` | quat | Optional. Local orientation (**authoritative**); absent = identity (0, 0, 0, 1). Field order: x, y, z, w |
| `scale` | vec3 | Optional. Local scale; absent = (1, 1, 1) |

All fields optional; an absent field means identity, so hosts may emit only what differs. Serialize at most these three fields. Do not serialize derived values a host may keep beside them (editor Euler angles, world matrix cache) — rebuild those from local pose + parent.

## Quat + Euler (hosts)

- **`rotation` (quat)** is the portable / composition source of truth — avoids gimbal lock for accumulated pose.
- Hosts **may** keep an editor **Euler** cache (radians) for property panels. That cache is not portable Rig data — do not serialize it.
- When the editor writes Euler → update `rotation` from it.
- When code / gizmos write `rotation` → refresh the Euler cache for display (`eulerAngles`).
- Present / hierarchy must build the local matrix from **`rotation`**, not from Euler.

2D hosts may use z = 0 and identity quat.
