# `rig.geometry.arc`

Circular arc or pie wedge. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `cx` | float | Centre X, local space |
| `cy` | float | Centre Y, local space |
| `radius` | float | Arc radius |
| `startAngleDegrees` | float | Start angle, 0 along +X, increasing toward +Y |
| `endAngleDegrees` | float | End angle, same convention |
| `pie` | bool | Optional. Absent = false; true closes the contour through the centre |

Angles are degrees, not radians, matching `fovYDegrees` on [`rig.spatial.camera`](../spatial/camera.md).

Sweep runs from start to end in increasing angle. An end angle below the start wraps past 360; the values are not normalized, so a 720-degree sweep is a legal double loop and a host that cannot draw one clamps rather than takes the modulus.

Elliptical arcs are not covered here — use [`rig.geometry.path`](path.md).

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
