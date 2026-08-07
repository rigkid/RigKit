/**
 * Emit schemas/json/rig.*.schema.json from an inline catalog.
 * Run: node tools/gen-schemas.mjs
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const outDir = path.join(root, "schemas", "json");

const ref = (name) => ({ $ref: `./_defs.schema.json#/$defs/${name}` });

function objectSchema(id, properties, opts = {}) {
  const required = opts.required ?? Object.keys(properties);
  const schema = {
    $schema: "https://json-schema.org/draft/2020-12/schema",
    $id: `https://rig.works/schemas/${id}.schema.json`,
    title: id,
    type: "object",
    additionalProperties: false,
    properties,
  };
  if (required.length) schema.required = required;
  if (opts.description) schema.description = opts.description;
  if (opts.allOf) schema.allOf = opts.allOf;
  if (opts.oneOf) schema.oneOf = opts.oneOf;
  if (opts.minProperties !== undefined) schema.minProperties = opts.minProperties;
  return schema;
}

function enumOf(values) {
  return { type: "string", enum: values };
}

/** @type {Record<string, object>} */
const catalog = {};

function add(id, properties, opts) {
  catalog[id] = objectSchema(id, properties, opts);
}

// --- spatial ---
// All fields optional; absent means identity (position 0,0,0 / rotation
// 0,0,0,1 / scale 1,1,1). Emit only what differs from identity if you like.
add("rig.spatial.transform", {
  position: ref("vec3"),
  rotation: ref("quat"),
  scale: ref("vec3"),
}, { required: [] });

add(
  "rig.spatial.relationship",
  {
    parent: ref("entity"),
    order: ref("int"),
  },
  { required: ["parent"] }
);

add(
  "rig.spatial.group",
  {},
  {
    required: [],
    description: "Marker: presence means scene group root. No fields.",
    minProperties: 0,
  }
);

// Defaults: active true, projection perspective, fovYDegrees 60,
// nearClip 0.1, farClip 1000, aspect = host viewport.
add("rig.spatial.camera", {
  active: ref("bool"),
  projection: enumOf(["perspective", "orthographic"]),
  fovYDegrees: ref("float"),
  orthoHeight: ref("float"),
  nearClip: ref("float"),
  farClip: ref("float"),
  aspect: ref("float"),
}, { required: [] });

// Defaults: order 0, visible true, locked false, rgba = no label colour.
add("rig.spatial.layer", {
  order: ref("int"),
  visible: ref("bool"),
  locked: ref("bool"),
  rgba: ref("rgba"),
}, { required: [] });

// --- layout ---
// Edge arrays run top, right, bottom, left (CSS order).
const edges = {
  type: "array",
  items: { type: "number" },
  minItems: 4,
  maxItems: 4,
};

add("rig.layout.page", {
  index: ref("int"),
  width: ref("float"),
  height: ref("float"),
  unit: ref("string"),
  margins: edges,
  bleed: edges,
  slug: edges,
}, { required: ["width", "height"] });

// --- geometry ---
// One schema per primitive family. A tagged union forced every shape to carry
// every other shape's fields; these carry only what they mean.
add("rig.geometry.rectangle", {
  x: ref("float"),
  y: ref("float"),
  width: ref("float"),
  height: ref("float"),
  cornerRadius: ref("float"),
}, { required: ["x", "y", "width", "height"] });

add("rig.geometry.ellipse", {
  cx: ref("float"),
  cy: ref("float"),
  rx: ref("float"),
  ry: ref("float"),
});

add("rig.geometry.line", {
  x1: ref("float"),
  y1: ref("float"),
  x2: ref("float"),
  y2: ref("float"),
});

add("rig.geometry.polygon", {
  points: { type: "array", items: ref("vec2"), minItems: 3 },
  closed: ref("bool"),
}, { required: ["points"] });

add("rig.geometry.regular_polygon", {
  cx: ref("float"),
  cy: ref("float"),
  radius: ref("float"),
  sides: { type: "integer", minimum: 3 },
  rotationDegrees: ref("float"),
}, { required: ["cx", "cy", "radius", "sides"] });

add("rig.geometry.star", {
  cx: ref("float"),
  cy: ref("float"),
  radius: ref("float"),
  innerRadius: ref("float"),
  points: { type: "integer", minimum: 3 },
  rotationDegrees: ref("float"),
}, { required: ["cx", "cy", "radius", "innerRadius", "points"] });

add("rig.geometry.arc", {
  cx: ref("float"),
  cy: ref("float"),
  radius: ref("float"),
  startAngleDegrees: ref("float"),
  endAngleDegrees: ref("float"),
  pie: ref("bool"),
}, { required: ["cx", "cy", "radius", "startAngleDegrees", "endAngleDegrees"] });

add("rig.geometry.ring", {
  cx: ref("float"),
  cy: ref("float"),
  outerRadius: ref("float"),
  innerRadius: ref("float"),
});

const pathCommand = {
  type: "object",
  additionalProperties: false,
  required: ["type"],
  properties: {
    type: enumOf(["moveTo", "lineTo", "cubicTo", "quadTo", "close"]),
    p: ref("vec2"),
    c1: ref("vec2"),
    c2: ref("vec2"),
  },
};

add("rig.geometry.path", {
  commands: { type: "array", items: pathCommand },
});

add("rig.geometry.mesh", {
  positions: {
    oneOf: [
      { type: "array", items: { type: "number" } },
      { type: "array", items: ref("vec3") },
    ],
  },
  indices: { type: "array", items: ref("uint32") },
  texcoords: {
    oneOf: [
      { type: "array", items: { type: "number" } },
      { type: "array", items: ref("vec2") },
    ],
  },
  mode: enumOf(["triangles", "lines", "lineStrip"]),
  faceColors: { type: "array", items: ref("rgba") },
  facePalette: { type: "array", items: ref("uint8") },
}, { required: ["positions", "mode"] });

// --- paint ---
// Defaults: hasFill = fillRgba present, hasStroke = strokeRgba present,
// strokeWidth 1.
add("rig.paint.fill_stroke", {
  fillRgba: ref("rgba"),
  strokeRgba: ref("rgba"),
  strokeWidth: ref("float"),
  hasFill: ref("bool"),
  hasStroke: ref("bool"),
}, { required: [] });

add("rig.paint.solid", {
  rgba: ref("rgba"),
  cmyk: ref("vec4"),
}, { required: ["rgba"] });

// p0/p1 default to (0,0) -> (1,0) in object space (SVG's default axis).
add("rig.paint.gradient", {
  kind: enumOf(["linear", "radial"]),
  stops: {
    type: "array",
    items: {
      type: "object",
      additionalProperties: false,
      required: ["t", "rgba"],
      properties: {
        t: { type: "number", minimum: 0, maximum: 1 },
        rgba: ref("rgba"),
      },
    },
  },
  p0: ref("vec2"),
  p1: ref("vec2"),
}, { required: ["kind", "stops"] });

// Paint by reference — several drawables sharing one paint entity. The inline
// spelling stays on rig.paint.fill_stroke; an entity uses one or the other.
add("rig.paint.fill", {
  paint: ref("entity"),
}, { required: ["paint"] });

add("rig.paint.stroke", {
  paint: ref("entity"),
  width: ref("float"),
}, { required: ["paint"] });

add("rig.paint.library", {
  paints: { type: "array", items: ref("entity") },
}, { required: ["paints"] });

// --- meta / render ---
add("rig.meta.named", {
  name: ref("string"),
  stableId: ref("string"),
}, { required: ["name"] });

add("rig.meta.tags", {
  tags: { type: "array", items: ref("string") },
});

// Defaults: enabled true, type point, rgb white, intensity 1, ambient 0,
// banded false.
add("rig.render.light", {
  enabled: ref("bool"),
  type: enumOf(["directional", "point"]),
  rgb: ref("rgb"),
  intensity: ref("float"),
  ambient: ref("float"),
  banded: ref("bool"),
  bands: ref("int"),
}, { required: [] });

// Defaults: albedoRgb white, metallic 0, roughness 1, emissive black.
add("rig.render.material", {
  albedoRgb: ref("rgb"),
  albedoMap: ref("entity"),
  metallic: ref("float"),
  roughness: ref("float"),
  emissive: ref("rgb"),
}, { required: [] });

// Per-entity show/hide. Draw order is sibling order — see rig.spatial.relationship.
add("rig.render.visibility", {
  visible: ref("bool"),
}, { required: ["visible"] });

// --- anim / mod ---
// Defaults: elapsed 0, easing linear, loop false, playing true.
add("rig.anim.tween", {
  target: ref("entity"),
  propertyKey: ref("string"),
  from: ref("float"),
  to: ref("float"),
  duration: ref("float"),
  elapsed: ref("float"),
  easing: enumOf(["linear", "easeIn", "easeOut", "easeInOut"]),
  loop: ref("bool"),
  playing: ref("bool"),
}, { required: ["target", "propertyKey", "from", "to", "duration"] });

// Defaults: amplitude 1, offset 0, phase 0.
add("rig.mod.lfo", {
  waveform: enumOf(["sine", "tri", "saw", "square"]),
  frequency: ref("float"),
  amplitude: ref("float"),
  offset: ref("float"),
  phase: ref("float"),
}, { required: ["waveform", "frequency"] });

// Defaults: depth 1, additive false; min/max absent = unclamped.
add("rig.mod.binding", {
  source: ref("entity"),
  target: ref("entity"),
  propertyKey: ref("string"),
  depth: ref("float"),
  min: ref("float"),
  max: ref("float"),
  additive: ref("bool"),
}, { required: ["source", "target", "propertyKey"] });

// --- music ---
// Defaults: playing false, timeSig 4/4, positionBeats 0, loop false.
add("rig.music.transport", {
  playing: ref("bool"),
  bpm: ref("float"),
  timeSigNum: ref("int"),
  timeSigDen: ref("int"),
  positionBeats: ref("float"),
  loop: ref("bool"),
  loopStartBeats: ref("float"),
  loopEndBeats: ref("float"),
}, { required: ["bpm"] });

// Defaults: phaseTicks 0, swingAmount 0, swingSubdiv 2, externalSync false.
add("rig.music.clock", {
  ticksPerQuarter: ref("int"),
  phaseTicks: ref("float"),
  swingAmount: ref("float"),
  swingSubdiv: ref("int"),
  externalSync: ref("bool"),
  syncBeat: ref("float"),
  syncPhase: ref("float"),
  syncPeriodBars: ref("float"),
}, { required: ["ticksPerQuarter"] });

// Defaults: pitch 60, velocity 100, gate 1, waveform 0, effect 0.
// waveform 0-7 are the portable synth shapes; 8+ are host instrument slots
// (a PICO-8 nibble 8-F references sfx 0-7 as a custom instrument).
add("rig.music.step", {
  active: ref("bool"),
  pitch: { type: "integer", minimum: 0, maximum: 127 },
  velocity: { type: "integer", minimum: 0, maximum: 127 },
  gate: { type: "number", minimum: 0, maximum: 1 },
  waveform: { type: "integer", minimum: 0, maximum: 255 },
  effect: { type: "integer", minimum: 0, maximum: 7 },
}, { required: ["active"] });

// Defaults: rootNote 60, scale chromatic, stepsPerBeat 4 (16th notes),
// loopStartStep 0, loopEndStep = steps.length (loop the whole pattern).
add("rig.music.pattern", {
  steps: {
    type: "array",
    items: { $ref: "./rig.music.step.schema.json" },
  },
  rootNote: { type: "integer", minimum: 0, maximum: 127 },
  scale: enumOf(["chromatic", "major", "minor", "dorian", "pentatonic"]),
  // Patterns sharing a sequencer stack by lane; absent means the only lane.
  lane: ref("int"),
  // Musical rate: steps advanced per transport beat.
  stepsPerBeat: { type: "number", exclusiveMinimum: 0 },
  loopStartStep: ref("int"),
  loopEndStep: ref("int"),
}, { required: ["steps"] });

// Defaults: currentStep 0; clock absent = the host's default clock.
add("rig.music.sequencer", {
  pattern: ref("entity"),
  currentStep: ref("int"),
  clock: ref("entity"),
}, { required: ["pattern"] });

// Defaults: velocity 100, channel 0; clip absent = free note.
add("rig.music.note", {
  pitch: { type: "integer", minimum: 0, maximum: 127 },
  velocity: { type: "integer", minimum: 0, maximum: 127 },
  channel: { type: "integer", minimum: 0, maximum: 15 },
  start: ref("float"),
  duration: ref("float"),
  clip: ref("entity"),
}, { required: ["pitch", "start", "duration"] });

// Ports carry whichever identifier the host API exposes: portName when it
// names ports, portIndex when it numbers them. Both are machine-local.
// At least one must be present. Default: open true.
const portIdentifier = [{ anyOf: [{ required: ["portName"] }, { required: ["portIndex"] }] }];

add("rig.music.midi_output", {
  portName: ref("string"),
  portIndex: ref("uint"),
  open: ref("bool"),
}, { required: [], allOf: portIdentifier });

add("rig.music.midi_input", {
  portName: ref("string"),
  portIndex: ref("uint"),
  open: ref("bool"),
  channel: { type: "integer", minimum: 0, maximum: 15 },
}, { required: [], allOf: portIdentifier });

// --- audio ---
// The analysis request, not its results. Band magnitudes are host state.
add("rig.audio.analysis", {
  source: enumOf(["input", "asset"]),
  asset: ref("entity"),
  bandCount: ref("int"),
  smoothing: { type: "number", minimum: 0, maximum: 1 },
  onsetThreshold: ref("float"),
}, { required: ["source", "bandCount"] });

// --- media ---
add("rig.media.asset_ref", {
  kind: enumOf(["image", "audio", "video", "model", "font", "other"]),
  path: ref("string"),
  loop: ref("bool"),
}, { required: ["kind", "path"] });

// Defaults: font absent = host default face, fontSize host default,
// rgba black, useKerning true, axes/features absent.
const textAxis = {
  type: "object",
  additionalProperties: false,
  required: ["tag", "value"],
  properties: {
    tag: ref("uint32"),
    value: ref("float"),
  },
};
add("rig.media.text", {
  text: ref("string"),
  font: ref("entity"),
  fontSize: ref("float"),
  rgba: ref("rgba"),
  axes: { type: "array", items: textAxis },
  features: ref("string"),
  useKerning: ref("bool"),
}, { required: ["text"] });

// Defaults: language absent = plain text, readOnly false.
add("rig.media.code", {
  text: ref("string"),
  language: ref("string"),
  readOnly: ref("bool"),
}, { required: ["text"] });

// --- pixel ---
// Default: clearRgba transparent black (0,0,0,0).
add("rig.pixel.canvas", {
  width: ref("int"),
  height: ref("int"),
  clearRgba: ref("rgba"),
}, { required: ["width", "height"] });

// Only the fields the chosen kind needs; the rest stay absent.
add("rig.pixel.source", {
  kind: enumOf(["none", "imageFile", "generator", "imageSequence", "webcam", "videoFile"]),
  asset: ref("entity"),
  generatorName: ref("string"),
  sequenceFps: ref("float"),
  sequenceFrame: ref("int"),
  webcamDevice: ref("int"),
  webcamWidth: ref("int"),
  webcamHeight: ref("int"),
  videoTime: ref("float"),
}, { required: ["kind"] });

// Defaults: blendMode normal, opacity 1, maskSource none, invertMask false.
add("rig.pixel.layer", {
  kind: enumOf(["vector", "overlayImage", "solid", "group"]),
  blendMode: enumOf(["normal", "multiply", "screen", "overlay", "add"]),
  opacity: { type: "number", minimum: 0, maximum: 1 },
  image: ref("entity"),
  rgba: ref("rgba"),
  maskSource: enumOf(["none", "luma", "alpha", "path"]),
  maskAsset: ref("entity"),
  maskLayer: ref("entity"),
  maskPathEntity: ref("entity"),
  invertMask: ref("bool"),
  groupParent: ref("entity"),
}, { required: ["kind"] });

add("rig.pixel.raster", {
  role: enumOf(["working", "output", "layerPixels", "mask", "composite"]),
  width: ref("int"),
  height: ref("int"),
  rgba: { type: "array", items: ref("uint8") },
});

add("rig.pixel.palette", {
  colors: { type: "array", items: ref("rgba"), minItems: 1 },
});

// flags absent = all zero.
add("rig.pixel.tile_set", {
  palette: ref("entity"),
  tileWidth: ref("int"),
  tileHeight: ref("int"),
  tilesAcross: ref("int"),
  tileRows: ref("int"),
  indices: { type: "array", items: ref("uint8") },
  flags: { type: "array", items: ref("uint8") },
}, { required: ["palette", "tileWidth", "tileHeight", "tilesAcross", "tileRows", "indices"] });

// origin: where this map's region sits inside a larger host map (default 0,0).
add("rig.pixel.tile_map", {
  tileSet: ref("entity"),
  width: ref("int"),
  height: ref("int"),
  originX: ref("int"),
  originY: ref("int"),
  tiles: { type: "array", items: ref("int") },
}, { required: ["tileSet", "width", "height", "tiles"] });

const effectStep = {
  type: "object",
  additionalProperties: false,
  required: ["id", "stage", "effectId"],
  properties: {
    id: ref("uint"),
    stage: enumOf(["image", "draw", "generate"]),
    effectId: ref("string"),
    enabled: ref("bool"),
    parentStep: ref("uint"),
  },
};

// Defaults: step enabled true; nextId = highest step id + 1.
add("rig.pixel.effect_chain", {
  steps: { type: "array", items: effectStep },
  nextId: ref("uint"),
}, { required: ["steps"] });

// --- io / led / sensor / interact / input ---
// Defaults: player 0, all buttons false.
add("rig.input.buttons", {
  player: ref("int"),
  left: ref("bool"),
  right: ref("bool"),
  up: ref("bool"),
  down: ref("bool"),
  o: ref("bool"),
  x: ref("bool"),
}, { required: [] });

// Listen and send sides are independent; enable one, both, or neither.
// listenPort is required when listenEnabled; sendHost + sendPort when
// sendEnabled. Defaults: both sides disabled.
add("rig.io.osc", {
  listenEnabled: ref("bool"),
  listenPort: ref("int"),
  sendEnabled: ref("bool"),
  sendHost: ref("string"),
  sendPort: ref("int"),
  addressPrefix: ref("string"),
}, { required: [] });

// Defaults: baud 9600, enabled false.
add("rig.io.serial", {
  port: ref("string"),
  baud: ref("int"),
  enabled: ref("bool"),
}, { required: ["port"] });

// E1.31 / sACN pixel output. Pairs with rig.led.uv_map for the pixel layout.
// Defaults: startChannel 1, enabled true, host = multicast for the universe.
add("rig.io.sacn", {
  universe: { type: "integer", minimum: 1, maximum: 63999 },
  startChannel: { type: "integer", minimum: 1, maximum: 512 },
  host: ref("string"),
  fps: ref("float"),
  enabled: ref("bool"),
  uvMap: ref("entity"),
}, { required: ["universe"] });

// width/height absent = normalized against the host output size.
add("rig.led.uv_map", {
  width: ref("int"),
  height: ref("int"),
  pixels: {
    type: "array",
    items: {
      type: "object",
      additionalProperties: false,
      required: ["index", "u", "v"],
      properties: {
        index: ref("int"),
        u: ref("float"),
        v: ref("float"),
      },
    },
  },
}, { required: ["pixels"] });

// Defaults: level 0; device absent = the host's default GPIO device.
add("rig.sensor.gpio", {
  pin: ref("int"),
  mode: enumOf(["in", "out"]),
  level: { type: "number", minimum: 0, maximum: 1 },
  device: ref("entity"),
}, { required: ["pin", "mode"] });

// --- sim ---
// Authored initial conditions and constants. Positions integrated at runtime
// belong to rig.spatial.transform; per-particle state is never serialized.
// Defaults: velocity (0,0,0), mass 1, drag 0; gravity absent = the host's
// ambient gravity (a per-body override, not a required constant).
add("rig.sim.rigidbody", {
  velocity: ref("vec3"),
  mass: ref("float"),
  drag: { type: "number", minimum: 0, maximum: 1 },
  gravity: ref("vec3"),
}, { required: [] });

add("rig.sim.particle_emitter", {
  rate: ref("float"),
  lifetime: ref("float"),
  maxParticles: ref("int"),
  gravity: ref("vec3"),
  damping: { type: "number", minimum: 0, maximum: 1 },
  startRgba: ref("rgba"),
  endRgba: ref("rgba"),
}, { required: ["rate", "lifetime", "maxParticles"] });

// Presence means selectable; enabled defaults true.
add("rig.interact.selectable", {
  enabled: ref("bool"),
}, { required: [], minProperties: 0 });

// --- ui (control surfaces: desktop, web, ESP) ---
// Defaults: order 0, visible true. role / preferred* are optional.
add(
  "rig.ui.panel",
  {
    role: ref("string"),
    order: ref("int"),
    visible: ref("bool"),
    preferredWidth: ref("float"),
    preferredHeight: ref("float"),
  },
  {
    required: [],
    description:
      "Portable tool surface. role is a stable tool id; preferred sizes are advisory. Dock/chrome is fulfillment.",
  }
);

// Defaults: orientation vertical, collapsed false, parent null (top-level in panel).
add(
  "rig.ui.group",
  {
    panel: ref("entity"),
    parent: ref("entity"),
    order: ref("int"),
    orientation: enumOf(["vertical", "horizontal"]),
    collapsed: ref("bool"),
  },
  {
    required: ["panel", "order"],
    description:
      "Section/row inside a panel. Nesting via parent (group entity). Flow hints are advisory.",
  }
);

add(
  "rig.ui.control",
  {
    panel: ref("entity"),
    group: ref("entity"),
    order: ref("int"),
    target: ref("entity"),
    propertyKey: ref("string"),
    type: ref("propertyType"),
    min: ref("float"),
    max: ref("float"),
    step: ref("float"),
    enabled: ref("bool"),
    readOnly: ref("bool"),
    options: { type: "array", items: ref("string") },
    widget: enumOf([
      "auto",
      "slider",
      "knob",
      "toggle",
      "field",
      "dropdown",
      "color",
      "xy",
    ]),
  },
  {
    required: ["panel", "order", "target", "propertyKey", "type"],
    description:
      "View over one POD field — never a second store. group is optional; widget is an advisory hint a host may ignore.",
  }
);

add(
  "rig.ui.action",
  {
    panel: ref("entity"),
    group: ref("entity"),
    order: ref("int"),
    actionId: ref("string"),
    enabled: ref("bool"),
  },
  {
    required: ["panel", "order", "actionId"],
    description:
      "Command button. Prefer shared actionId names for portable tools; unknown ids may be hidden. Most likely to change before 1.0.0.",
  }
);

// --- node (nested types first as standalone schemas for $ref) ---
add("rig.node.pin", {
  id: ref("uint"),
  name: ref("string"),
  kind: enumOf(["in", "out"]),
  type: ref("propertyType"),
});

// rig.node.param is defined below as a oneOf-only tagged union.

add("rig.node.link", {
  id: ref("uint"),
  fromNode: ref("uint"),
  fromPin: ref("uint"),
  toNode: ref("uint"),
  toPin: ref("uint"),
});

add("rig.node.publish", {
  pin: ref("uint"),
  innerNode: ref("uint"),
  innerPin: ref("uint"),
});

// node + graph: mutual recursion via $ref
catalog["rig.node.node"] = {
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "https://rig.works/schemas/rig.node.node.schema.json",
  title: "rig.node.node",
  type: "object",
  additionalProperties: false,
  required: ["id", "typeId", "title", "pos", "pins", "params"],
  properties: {
    id: ref("uint"),
    typeId: ref("string"),
    title: ref("string"),
    pos: ref("vec2"),
    pins: { type: "array", items: { $ref: "./rig.node.pin.schema.json" } },
    params: { type: "array", items: { $ref: "./rig.node.param.schema.json" } },
    nested: { $ref: "./rig.node.graph.schema.json" },
    publishes: {
      type: "array",
      items: { $ref: "./rig.node.publish.schema.json" },
    },
  },
};

catalog["rig.node.graph"] = {
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "https://rig.works/schemas/rig.node.graph.schema.json",
  title: "rig.node.graph",
  type: "object",
  additionalProperties: false,
  required: ["nodes", "links", "nextId"],
  properties: {
    nodes: { type: "array", items: { $ref: "./rig.node.node.schema.json" } },
    links: { type: "array", items: { $ref: "./rig.node.link.schema.json" } },
    nextId: ref("uint"),
  },
};

// Param is a tagged union: only the storage field selected by `type` may be present,
// so it is oneOf-only rather than a properties/required object schema.
catalog["rig.node.param"] = {
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "https://rig.works/schemas/rig.node.param.schema.json",
  title: "rig.node.param",
  description:
    "Serialize key, type, and only the storage field selected by type (f/i/s/v).",
  oneOf: [
    {
      type: "object",
      additionalProperties: false,
      required: ["key", "type", "f"],
      properties: {
        key: ref("string"),
        type: { const: "float" },
        f: ref("float"),
      },
    },
    {
      type: "object",
      additionalProperties: false,
      required: ["key", "type", "f"],
      properties: {
        key: ref("string"),
        type: { const: "double" },
        f: ref("float"),
      },
    },
    {
      type: "object",
      additionalProperties: false,
      required: ["key", "type", "i"],
      properties: {
        key: ref("string"),
        type: { enum: ["bool", "int", "uint", "enum", "entity"] },
        i: ref("int"),
      },
    },
    {
      type: "object",
      additionalProperties: false,
      required: ["key", "type", "s"],
      properties: {
        key: ref("string"),
        type: { const: "string" },
        s: ref("string"),
      },
    },
    {
      type: "object",
      additionalProperties: false,
      required: ["key", "type", "v"],
      properties: {
        key: ref("string"),
        type: { enum: ["vec2", "vec3", "vec4", "quat"] },
        v: ref("vec4"),
      },
    },
  ],
};

// Document envelope
const componentIds = Object.keys(catalog).filter((id) => id !== "_defs").sort();
const componentMapProperties = Object.fromEntries(
  componentIds.map((id) => [id, { $ref: `./${id}.schema.json` }])
);

catalog["rig.document"] = {
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "https://rig.works/schemas/rig.document.schema.json",
  title: "rig.document",
  description:
    "Rig JSON document envelope. Component keys must be rig.<domain>.<name> schema ids.",
  type: "object",
  additionalProperties: false,
  required: ["rig", "entities"],
  properties: {
    rig: {
      type: "string",
      description: "Contract version this document targets (e.g. 0.9.0).",
      pattern: "^[0-9]+\\.[0-9]+\\.[0-9]+$",
    },
    document: {
      type: "object",
      additionalProperties: true,
      properties: {
        title: { type: "string" },
        author: { type: "string" },
        createdAt: { type: "string" },
        modifiedAt: { type: "string" },
        defaultUnit: { type: "string" },
        colorSpace: {
          type: "string",
          description: "Colour space for all rgba/rgb values (default: srgb).",
        },
      },
    },
    entities: {
      type: "array",
      items: {
        type: "object",
        additionalProperties: false,
        required: ["id", "components"],
        properties: {
          id: {
            type: "string",
            minLength: 1,
            description: "Stable within this file; entity-typed fields reference these ids.",
          },
          components: {
            type: "object",
            propertyNames: {
              // Contract ids, or x.<vendor>.<name> for host components the
              // Contract has not named. Extensions travel; they do not port.
              pattern: "^(rig|x)\\.[a-z0-9_]+\\.[a-z0-9_]+$",
            },
            additionalProperties: true,
            properties: componentMapProperties,
          },
        },
      },
    },
  },
};


// Shared POD datatype defs (previously hand-maintained as schemas/json/_defs.schema.json)
catalog["_defs"] = {
  $schema: "https://json-schema.org/draft/2020-12/schema",
  $id: "https://rig.works/schemas/_defs.schema.json",
  $comment: "Shared property datatypes for Rig POD JSON interchange.",
  $defs: {
    bool: { type: "boolean" },
    int: { type: "integer" },
    uint: { type: "integer", minimum: 0 },
    uint8: { type: "integer", minimum: 0, maximum: 255 },
    uint32: { type: "integer", minimum: 0, maximum: 4294967295 },
    float: { type: "number" },
    double: { type: "number" },
    string: { type: "string" },
    vec2: { type: "array", minItems: 2, maxItems: 2, items: { type: "number" } },
    vec3: { type: "array", minItems: 3, maxItems: 3, items: { type: "number" } },
    vec4: { type: "array", minItems: 4, maxItems: 4, items: { type: "number" } },
    quat: {
      type: "array",
      minItems: 4,
      maxItems: 4,
      items: { type: "number" },
      $comment: "Order: x, y, z, w",
    },
    rgba: {
      type: "array",
      minItems: 4,
      maxItems: 4,
      items: { type: "number", minimum: 0, maximum: 1 },
    },
    rgb: {
      type: "array",
      minItems: 3,
      maxItems: 3,
      items: { type: "number", minimum: 0, maximum: 1 },
    },
    entity: {
      type: ["string", "null"],
      $comment: "Entity id within the document; null = none / unset",
    },
    propertyType: {
      type: "string",
      description: "Property datatype id from docs/properties.md (or host-prefixed).",
    },
  },
};

const checkOnly = process.argv.includes("--check");

// Working trees with core.autocrlf check these files out as CRLF; compare content, not bytes.
const normalize = (text) => text.replace(/\r\n/g, "\n");

if (checkOnly) {
  const stale = [];
  for (const [id, schema] of Object.entries(catalog)) {
    const file = path.join(outDir, `${id}.schema.json`);
    const expected = JSON.stringify(schema, null, 2) + "\n";
    const actual = fs.existsSync(file) ? normalize(fs.readFileSync(file, "utf8")) : null;
    if (actual !== expected) stale.push(path.relative(root, file));
  }
  if (stale.length) {
    console.error("schemas/json/ is out of date — run: node tools/gen-schemas.mjs");
    for (const file of stale) console.error("  ", file);
    process.exit(1);
  }
  console.log(`schemas up to date — ${Object.keys(catalog).length} schemas`);
} else {
  fs.mkdirSync(outDir, { recursive: true });
  for (const [id, schema] of Object.entries(catalog)) {
    const file = path.join(outDir, `${id}.schema.json`);
    fs.writeFileSync(file, JSON.stringify(schema, null, 2) + "\n");
    console.log("wrote", path.relative(root, file));
  }
  console.log(`done — ${Object.keys(catalog).length} schemas`);
}
