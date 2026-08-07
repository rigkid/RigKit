# `rig.layout.page`

A page, artboard, or frame — a bounded region content is composed against. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `index` | int | Optional. Position in the page sequence, ascending; absent = 0. Ties break by document order |
| `width` | float | Page width in `unit` |
| `height` | float | Page height in `unit` |
| `unit` | string | Optional. Overrides the document `defaultUnit` for this page (`"mm"`, `"px"`, `"in"`) |
| `margins` | number[4] | Optional. Top, right, bottom, left — CSS order. Absent = no margins |
| `bleed` | number[4] | Optional. Same order. Extent content may run past the trim edge |
| `slug` | number[4] | Optional. Same order. Notes area outside the bleed, discarded at print |

Margins, bleed, and slug are prepress vocabulary, but a page is not only print — an artboard or a fixed-size frame is the same concept with the arrays omitted. `width` and `height` describe the trim size; bleed and slug extend outward from it.

Name the page by composing [`rig.meta.named`](../meta/named.md). Content belongs to a page via [`rig.spatial.relationship`](../spatial/relationship.md), the same as any other parent.

Which page is currently open in an editor is host state, not a field here.
