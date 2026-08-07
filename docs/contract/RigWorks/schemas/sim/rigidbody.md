# `rig.sim.rigidbody`

Point-mass physics constants. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `velocity` | vec3 | Optional. Initial velocity, units per second; absent = at rest |
| `mass` | float | Optional. Mass; must be greater than 0; absent = 1 |
| `drag` | float | Optional. 0–1, fraction of velocity lost per second; absent = 0 |
| `gravity` | vec3 | Optional. Per-body acceleration, units per second squared; absent = the host's ambient gravity |

`gravity` is an override, not a required constant. A host that keeps one gravity vector for the whole simulation omits the field and applies its own; a host with per-body gravity writes it.

These are authored initial conditions, not live state. Position is integrated into [`rig.spatial.transform`](../spatial/transform.md) as the simulation runs, so a document saved mid-run captures where things are, and `velocity` still reads as where they started.

That means a simulation is not resumable across hosts. Reloading gives the same scene with the bodies re-launched, not the run frozen and continued. Rig carries the setup; the trajectory belongs to whoever integrates it.

Collision shapes, restitution, friction, and constraints are deliberately absent. No two engines agree on them closely enough for the numbers to mean the same thing twice.
