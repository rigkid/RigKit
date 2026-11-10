# Refresh the firmware copies of the scene and the page.
# device.rig becomes a C string. index.html becomes a gzipped byte array.
# Rewrites a file only when the bytes change, so a rebuild does not touch git.

import gzip
import json
import pathlib

root = pathlib.Path(__file__).resolve().parents[3]
scene = root / "examples" / "example-esp32" / "device.rig"
html = root / "examples" / "example-esp32" / "web" / "index.html"
inc = root / "firmware" / "esp" / "src" / "scene_default.inc"
header = root / "firmware" / "esp" / "src" / "web_ui_gz.inc"


def write_if_changed(path: pathlib.Path, text: str) -> None:
    existing = path.read_text(encoding="utf-8") if path.exists() else None
    if existing != text:
        path.write_text(text, encoding="utf-8", newline="\n")


def pack_scene() -> None:
    raw = scene.read_text(encoding="utf-8")
    write_if_changed(inc, json.dumps(raw) + "\n")


def pack_html() -> None:
    payload = gzip.compress(html.read_bytes(), compresslevel=9, mtime=0)
    lines = [
        "#pragma once",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "static const uint8_t kWebUiGz[] = {",
    ]
    for i in range(0, len(payload), 16):
        chunk = ", ".join(str(b) for b in payload[i : i + 16])
        lines.append(f"\t{chunk},")
    lines.append("};")
    lines.append("static const size_t kWebUiGzLen = sizeof(kWebUiGz);")
    lines.append("")
    write_if_changed(header, "\n".join(lines))


if __name__ == "__main__":
    pack_scene()
    pack_html()
