# example-kicadSch

A thin schematic sheet. The `.kicad_sch` stays the document. Symbol drawings come from the file's `lib_symbols`, then from KiCad's `sym-lib-table` and symbol libraries (`Device:R` and the rest). No KiCad process is linked.

Drag a symbol and its anchor snaps to the grid. Wire ends, junctions, and labels that sat on a pin move with the pin, so changing the grid does not drop a connection. `A` lists the KiCad symbol libraries; type a name (`Device:R`, `power:GND`), Enter, then click the page. Each click drops another copy until Esc.

Click a hierarchical sheet to open it. `H` lists every sheet in the project (the `.kicad_pro` next to the file, then the sheets it points at). Backspace returns to the sheet you came from. Edits stay in memory while you move around.

```bash
cmake -S examples/example-kicadSch -B examples/example-kicadSch/build
cmake --build examples/example-kicadSch/build --target example-kicadSch
./examples/example-kicadSch/build/bin/example-kicadSch path/to/sheet.kicad_sch
./examples/example-kicadSch/build/bin/example-kicadSch --smoke
```

With no path it opens `data/led.kicad_sch`.

| Key | |
|-----|--|
| drag | move a symbol; attached wire ends follow |
| A | place from the KiCad symbol libraries |
| type, up/down, Enter | pick a library, then a symbol |
| click | drop the armed symbol (click again for another) |
| click a sheet box | open that sheet |
| H | list the sheets in this project |
| Backspace | return to the previous sheet |
| Esc | close the list, then leave place mode |
| middle / right drag | pan |
| wheel | zoom |
| `[` `]` | grid 0.254 / 1.27 / 2.54 mm (nothing already placed moves) |
| F | fit the page |
| S | save (a `.bak` of the original is written once; the save is reprinted s-expression) |

No design-rule check. Text is a small bitmap, uppercase. The symbol drawing is copied into the sheet, so the file still opens where that library is not installed. Save writes the sheet you are looking at.
