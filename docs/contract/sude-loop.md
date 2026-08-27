# SUDE loop

**SUDE** = Setup / Update / Draw / Exit. **RigKit floor** - every RigKit host must honor it. Portable live-host convention in RigWorks; not required to “be Rig” ([Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md) is grammar + entity/component POD).

Portable source of truth: [Rig SUDE](https://github.com/rigkid/RigWorks/blob/main/docs/sude.md). This file is the RigKit working copy.

No window, renderer, UI kit, or language is required.

A host is **SUDE-compliant** if it calls application hooks in the order below and respects the semantics. RigKit’s `IApp` setup / update / draw hooks are one fulfillment, not the law.

## Hooks

| Hook | When | Semantics |
|------|------|-----------|
| `Setup()` | Once, after the host is ready | Allocate app state. May be empty. |
| `Update(dt)` | Each tick, before Draw | Simulation / logic. `dt` is seconds since last Update (host-owned clock). |
| `Draw()` | Each tick, after Update | Present. Always called. Body may be empty; the hook is not optional. |
| `Exit()` | Once, before teardown | Release app state. Recommended; may be empty. |

**Draw is present** - pixels, LEDs, GPIO, serial frames, whatever the piece puts into the world. Not “only if a GPU window exists.” This is software for art: without Draw there is no SUDE.

## Ordering

```
Setup, then (Update, Draw)*, then Exit
```

- Host must not nest hooks (no Update inside Draw that re-enters Update).
- Host **must call Draw every tick** after Update. Skipping Draw is not SUDE.
- The app may leave `Draw()` empty; the host still calls it.
- Host must not require a display, GPU, or UI library for SUDE compliance.

## Clock

- Host owns time and supplies `dt` to `Update`.
- Apps must not assume a fixed FPS unless they document that assumption.
- `dt` should be non-negative; hosts should clamp pathological spikes if needed for installs.

## Non-requirements (explicit)

The SUDE loop does **not** require:

- A UI pack
- A GPU present path or particular renderer
- ECS runtime (see [rigkit.md](rigkit.md) - document composition is still the Contract floor)
- Filesystem, networking, or audio
- A window

## Compliance examples

| Host | How it complies |
|------|-----------------|
| Desktop author | Setup, Update, Draw each tick; **rigImGui** is distribution, not SUDE |
| Pi kiosk / show mode | Same SUDE loop; Draw presents; UI pack unloaded or hidden |
| Headless CI | Setup, Update, Draw (empty body), then Exit |
| ESP32 firmware | Setup once; Update then Draw each tick (e.g. LED / GPIO in Draw) |

## Compliance

A project **is SUDE** when:

1. It implements or is driven by the four hooks (Exit optional but recommended).
2. The host calls `Draw()` every tick after `Update` - empty body is valid; skipping the call is not.
3. No SUDE requirement forces a UI or GPU dependency.

**RigKit** requires SUDE + ECS ([rigkit.md](rigkit.md)) - that is this host’s floor. **Being Rig** (RigWorks) is grammar + entity/component POD and does not require SUDE - [Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md).

See also: [rigkit.md](rigkit.md) (ECS), [ui.md](ui.md) (optional UI), [distribution.md](distribution.md), [README.md](README.md).
