# Examples

Canonical Rig JSON documents. Copy the pattern; validate before you trust the output. Agent distillation of these rules: [`../skills/generating-rig-documents/SKILL.md`](../skills/generating-rig-documents/SKILL.md).

```bash
node tools/rig-validate/cli.js examples/minimal-scene.json
node tools/rig-validate/cli.js examples/lfo-binding.json
node tools/rig-validate/cli.js examples/ui-panel.json
node tools/rig-validate/cli.js examples/portable-tool.json
```

Fantasy-console carts (PICO-8) live in [rigPico8](https://github.com/rigkid/rigPico8).

## SUDE sketch

Hosts honor Setup / Update / Draw / Exit. Data is ECS POD composed onto entities — the JSON below is what travels; the loop is how a host runs it.

```text
Setup:
  load or spawn entities from a Rig document
Update(dt):
  advance modulators, tweens, transport, bindings…
Draw:
  present transforms / shapes / meshes / whatever this host draws
Exit:
  release host state
```

## `minimal-scene.json`

A specimen sheet of the geometry vocabulary: rounded rectangle, circle, regular polygons (triangle, hexagon), star, parent/child hierarchy, closed and open polygons, line, and a quad mesh. Rendered to [`site/scene.svg`](../site/scene.svg) by [`tools/render-svg.mjs`](../tools/render-svg.mjs). Note what is *absent*: fill-only shapes carry just `fillRgba` and stroke-only shapes just `strokeRgba` — 0.8.0 defaults cover the rest.

One entity (excerpt):

<!-- rig:begin entity=demo-rect from=examples/minimal-scene.json -->
```json
{
  "id": "demo-rect",
  "components": {
    "rig.meta.named": {
      "name": "demo-rect",
      "stableId": "demo-rect"
    },
    "rig.spatial.transform": {
      "position": [
        20,
        65,
        0
      ],
      "rotation": [
        0,
        0,
        0,
        1
      ],
      "scale": [
        1,
        1,
        1
      ]
    },
    "rig.geometry.rectangle": {
      "x": 0,
      "y": 0,
      "width": 140,
      "height": 90,
      "cornerRadius": 10
    },
    "rig.paint.fill_stroke": {
      "fillRgba": [
        0.25,
        0.65,
        1,
        1
      ]
    },
    "rig.interact.selectable": {
      "enabled": true
    }
  }
}
```
<!-- rig:end -->

## `lfo-binding.json`

`rig.mod.lfo` on one entity, `rig.mod.binding` driving another entity’s `position.y` — Update-side data with no UI required.

## `ui-panel.json`

An LED-install flavoured control surface: `rig.ui.panel` (`role: led.install`) with controls driving `rig.paint.solid` colour and `rig.mod.lfo` frequency, plus a shared `lfo.resetPhase` action. Same schemas a desktop panel or an ESP32 web page would speak.

## `portable-tool.json`

A tool document meant to travel: panel + nested `rig.ui.group` sections, controls bound to LFO and paint fields, shared action id. Author in one UI pack; load in another app that speaks the same schema ids.
