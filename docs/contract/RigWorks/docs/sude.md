# SUDE loop

**SUDE** = Setup / Update / Draw / Exit.

No window, renderer, UI pack, or language is required.

## Hooks

| Hook | When | Semantics |
|------|------|-----------|
| `Setup()` | Once, after the host is ready | Allocate app state. May be empty. |
| `Update(dt)` | Each tick, before Draw | Simulation / logic. `dt` is seconds since last Update (host-owned clock). |
| `Draw()` | Each tick, after Update | Present. Always called. Body may be empty; the hook is not optional. |
| `Exit()` | Once, before teardown | Release app state. Recommended; may be empty. |

**Draw is present** — pixels, LEDs, GPIO, serial, sound, whatever the piece puts into the world.

## Ordering

```
Setup → ( Update → Draw )* → Exit
```

- Do not nest hooks.
- Call `Draw` every tick after `Update`. Skipping the call is not SUDE; an empty body is fine.
- Do not require a display or UI pack for SUDE compliance.

## Clock

- Host owns time and supplies `dt` to `Update`.
- Do not assume a fixed FPS unless documented.
- `dt` should be non-negative; clamp spikes on long-running installs when needed.

## Non-requirements

SUDE does not require a UI pack, a GPU present path, ECS, filesystem, networking, or audio.

**Rig** also requires [ECS](ecs.md). See [honors.md](honors.md).
