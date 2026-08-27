# Node graphs (artist guide)

Patch floats, vec2, colors, or any [standard datatype](contract/RigWorks/docs/properties.md) in the Node Editor.
The graph is plain data (`CNodeGraph`). Eval runs live while the window is open.

Example: [`packs/rigNodeEditor/examples/nodes`](../packs/rigNodeEditor/examples/nodes/).

```bash
cmake -S packs/rigNodeEditor/examples/nodes -B packs/rigNodeEditor/examples/nodes/build
cmake --build packs/rigNodeEditor/examples/nodes/build --target nodes
./packs/rigNodeEditor/examples/nodes/build/bin/nodes
```

Packs: **rigNodeComponent** (data + catalog + eval + `.rig` codecs) + **rigNodeEditor**
(window). Register **rigProject** for Open/Save Scene.

Apps can still use **Float Out** / **Vec2 Out** / **Color Out** (eval fills
`EvalResult::outputs` lists). Prefer **Ref** sinks to write scene properties by
entity name + property name - no hardcoded app index binding.

A 3D editor app can seed tiny **Ref** nodes:

| Ref | Entity | Property |
|-----|--------|----------|
| Float Ref | `camera` | `Speed` / `Radius` / `Height` (`COrbitDrive`) |
| Color Ref | `key-light` | Color R/G/B |

Drag a Scene entity onto the Node Editor canvas to spawn a Ref; pick the property
in the params row. Entities that receive graph values can carry `CDriveHint`
(shown at the top of Properties).

## Editor cheatsheet

| Action | How |
|--------|-----|
| Add a node | **File > Add node** - pick a category, or type in Search |
| Move a node | Drag the node body |
| Link | Click an **output** pin, then an **input** pin - or drag Out to In |
| Cancel a link | Esc, or click empty canvas (after a drag miss) |
| Select | Click a node (yellow outline) |
| Edit params | Select a node - params appear in **Properties** |
| Scene to Ref | Drag an entity from **Scene** onto the Node Editor canvas |
| Property to Ref | Drag a property row from **Properties** onto the canvas (binds entity + prop) |
| Property to Modulate | **Alt**-drop a float property, or **Drive with LFO** on a Float Ref, or **Modulate** in Properties |
| Delete | Select + **File > Delete node**, or Del |
| Pan | Middle-mouse drag, or Alt + left-drag |
| Zoom | Mouse wheel over the canvas |
| Demo graph | App startup: color wash (LFO to Map to Brightness to Color Out / Ref). **File > Seed demo** - Value(1) + Value(2) to Add to Float Out (= 3) |
| Save / load | **File > Save Scene...** / **Open Scene...** (needs `rigProject`) |
| Groups | **File > Empty group** / **Group selection** / **Ungroup** / **Publish pin** / **Up** |

Live values show on output pins while the graph runs. The Node Editor status bar shows
`t=` (seconds), **Float Out** results, and Ref bindings.

## Pin colors (types)

Pin `type` uses Rig property datatypes:

| Editor colour | `type` | Notes |
|---------------|--------|-------|
| Green | `float` | Scalar |
| Blue | `vec2` | XY |
| Pink | `vec4` | Often colour (RGBA); domain in pin/catalog name |

Pins connect on exact match, or host coercion (`float` / `vec2` / `vec4`). Empty `type` = wildcard.

## Nestable groups

A **group** is a node with a `nested` graph and `publishes` (outer pin to inner pin). It links like any node on the parent canvas.

| Editor | How |
|--------|-----|
| Empty group | **File > Empty group** |
| Group selection | Ctrl+click nodes, then **File > Group selection** (crossing links become publishes) |
| Ungroup | Select a group, then **File > Ungroup** (rewires via publishes) |
| Dive | Double-click a group, or **Dive into group** in Properties |
| Surface | **File > Up** |
| Publish | Inside a group, select a pin, then **File > Publish pin** |

Eval runs nested graphs recursively. While diving, live preview injects published inputs from the parent canvas.

## Built-in catalog

Search in **Add node**. `typeId` is what `.rig` stores.

### Values

| Title | typeId | Notes |
|-------|--------|-------|
| Value | `float.value` | Constant (param) |
| Time | `float.time` | Seconds since window open |
| Delta | `float.dt` | Frame delta |

### Maths

| Title | typeId |
|-------|--------|
| Add / Subtract / Multiply / Divide / Modulo / Power | `float.add` ... `float.pow` |
| Min / Max / Abs / Negate | `float.min` ... `float.neg` |
| Floor / Ceil / Fract | `float.floor` ... `float.fract` |
| Sin / Cos / Tan | `float.sin` ... `float.tan` |
| Clamp | `float.clamp` |
| Map | `float.map` |
| Mix / Step / SmoothStep | `float.mix` ... `float.smoothstep` |

### Modulators

| Title | typeId | Notes |
|-------|--------|-------|
| LFO | `mod.lfo` | Wave: Sine / Triangle / Saw / Square |
| Noise | `mod.noise` | Smooth noise |
| Random | `mod.random` | Stepped random |
| Ramp | `mod.ramp` | Looping ramp |
| Envelope | `mod.envelope` | ADSR; `gate` input |

### Logic

| Title | typeId |
|-------|--------|
| Compare | `logic.compare` |
| Gate | `logic.gate` |
| Switch | `logic.switch` |

### Vectors

| Title | typeId |
|-------|--------|
| Vec2 / Split / Join | `vec2.value` / `vec2.split` / `vec2.join` |
| Add / Sub / Mul / Scale | `vec2.add` ... `vec2.scale` |
| Length / Normalize / Dot | `vec2.length` ... `vec2.dot` |

### Color

| Title | typeId |
|-------|--------|
| Color / Split / Join | `color.value` / `color.split` / `color.join` |
| Mix Color / Brightness | `color.mix` / `color.brightness` |

### Output

| Title | typeId | Notes |
|-------|--------|-------|
| Float Out | `float.out` | App-readable list (`EvalResult::outputs`) |
| Vec2 Out | `vec2.out` | |
| Color Out | `color.out` | |

### Ref

| Title | typeId | Notes |
|-------|--------|-------|
| Float Ref | `ref.float` | Writes one float property (`entity` + `prop`) |
| Vec2 Ref | `ref.vec2` | Writes one vec2, or two floats (`prop` + `propY`) |
| Color Ref | `ref.color` | Writes Color R/G/B (optional name prefix) |

Call `applyRefWrites` is no longer required in app `update` - `SGraphEval`
(registered by **rigNodeComponent**) evaluates every `CNodeGraph` and applies Ref
writes each Update. Ref `entity` + `prop` is the same addressing as `rig.mod.binding`
(`target` + `propertyKey`). Contract modulators / tweens use `CModLfo` /
`CModBinding` / `CTween` + `SModulators` / `STweens`. Orbit framing uses
`COrbitDrive` + `SOrbitDrive`.

Editor dive preview still uses `evaluateAlongDive` in the Node Editor (no apply).

Unknown `typeId`s in a `.rig` are skipped at eval (no crash). Domain packs may add
more catalogs later; the built-in table above is what ships today.

## Recipes

### 1. Seed demo (sanity)

**File > Seed demo** - status bar should show Float Out `3`.

### 2. Breathing value (LFO)

1. Add **LFO** (Modulators). Frequency `0.5`, Amplitude `1`, Offset `0`.
2. Link LFO `out` to **Float Out** `in` (or a **Float Ref** to a scene property).
3. Watch the status bar / pin value oscillate.

Example binding: LFO, then Float Ref, then `camera` / `Speed` - tweak the LFO
in the Node Editor and drive orbit from Update.

### 3. Mapped pulse

1. **LFO** into **Map** (`inMin`/`inMax` −1..1, `outMin`/`outMax` 0..1), then **Float Out**.
2. Optional: **SmoothStep** after Map for softer edges.

### 4. Color wash (key light)

1. **Color** (base tint) into **Brightness** `in`.
2. **LFO** into **Brightness** `gain`.
3. **Brightness** `out` into **Color Out**.

Example binding: Color Ref / Color Out tints a key light.

### 5. Vec2 orbit path (orbit camera)

1. Two **LFO**s (slow) into **Join Vec2** (`x` / `y`), then **Vec2 Out**.
2. Or a **Vec2** node with X/Y params.

Example binding: Vec2 Out sets orbit radius (x) and height (y).

## Limits

- One graph entity is edited (first / selected `CNodeGraph` in the scene).
- Eval is CPU float/vec2/vec4 - not a shader graph.
- Open/Save needs **rigProject** registered; otherwise those buttons stay inert.
- Wiring outputs into transforms/materials is the **app’s** job each Update.
