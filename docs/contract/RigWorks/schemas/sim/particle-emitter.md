# `rig.sim.particle_emitter`

Particle spawn settings. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `rate` | float | Particles spawned per second |
| `lifetime` | float | Seconds each particle survives |
| `maxParticles` | int | Hard cap on live particles |
| `gravity` | vec3 | Optional. Acceleration applied to every particle |
| `damping` | float | Optional. 0–1, velocity lost per second |
| `startRgba` | rgba | Optional. Colour at spawn |
| `endRgba` | rgba | Optional. Colour at death; interpolated over the lifetime |

Particles spawn at the entity's own position from [`rig.spatial.transform`](../spatial/transform.md). There is no emitter shape — a volume or ring emitter is not portable, and a host that has one writes the settings it can and keeps the rest privately.

Live particles are never serialized. Reopening a document restarts the emitter empty rather than resuming mid-burst, so a screenshot of a running system will not reproduce exactly.

Seeds and randomness are not specified. Two hosts running the same emitter produce different particles on purpose; if you need a repeatable arrangement, author the entities instead.
