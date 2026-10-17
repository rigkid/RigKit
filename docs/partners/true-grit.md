# True Grit Texture Supply

Partnership path for Rig brush packs. Not a license to load their Photoshop `.abr` files.

Their [user license](https://www.truegrittexturesupply.com/pages/user-license-agreement-v2) limits installable assets to the apps a product was designed for. Photoshop brushes stay in Adobe apps. They already sell a separate Procreate SKU rebuilt for that engine. Rig needs the same: a **Rig SKU** sold on their store, installed via `brushes.json` + tip PNGs.

## Outreach draft

Subject: Rig brush SKU (not ABR import)

We host an open, Pi-capable editor (RigKit) where crafts are packs on one document - vector and pixel share a `.rig`, and App > Workspace switches chrome.

We will not load or redistribute True Grit Photoshop `.abr` packs. We want a Rig-native brush set the way you already ship Procreate: tips + settings built for our dab engine.

What we can offer:

- Install docs that name your product and link your store
- Empty-state / Brushes panel credit
- A stable pack format (`brushes.json` + PNG tips)

What we need from you:

- Preferred tip handoff (PNG stamps + a settings sheet)
- Whether the SKU lives only on your store
- Branding rules for the in-app credit

Until a written SKU exists we ship only original demo brushes.

## Format they would ship

```
pack-name/
  brushes.json
  tip-optional.png
```

`brushes.json` fields: `name`, `brushes[]` with `name`, `tip`, `size`, `opacity`, `hardness`, `spacing`, `scatter`, `angleJitter`, `flow`.
