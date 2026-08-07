#!/usr/bin/env node
/**
 * Still-frame SVG fulfillment of examples/minimal-scene.json.
 *
 * SUDE mapping for this tool (honest, incomplete loop):
 *   Setup — load the Rig document
 *   Draw  — emit SVG from the 2D primitives, mesh, transform, relationship, fill_stroke
 *   Exit  — write site/scene.svg
 *   (no Update — this is a still frame)
 *
 * Ships what it supports: rig.geometry.arc, .ring, and .path are skipped rather
 * than approximated.
 *
 *   node tools/render-svg.mjs           # write
 *   node tools/render-svg.mjs --check   # exit 1 if stale
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const src = path.join(root, "examples/minimal-scene.json");
const out = path.join(root, "site/scene.svg");
const checkOnly = process.argv.includes("--check");

const doc = JSON.parse(fs.readFileSync(src, "utf8"));
const byId = new Map(doc.entities.map((e) => [e.id, e]));

function comps(e) {
  return e.components || {};
}

function worldPos(id, seen = new Set()) {
  if (seen.has(id)) return [0, 0];
  seen.add(id);
  const e = byId.get(id);
  if (!e) return [0, 0];
  const t = comps(e)["rig.spatial.transform"];
  const local = t?.position ?? [0, 0, 0];
  const rel = comps(e)["rig.spatial.relationship"];
  if (rel?.parent) {
    const [px, py] = worldPos(rel.parent, seen);
    return [px + local[0], py + local[1]];
  }
  return [local[0], local[1]];
}

function rgba(arr, fallback = "none") {
  if (!arr || arr.length < 3) return fallback;
  const [r, g, b, a = 1] = arr;
  const R = Math.round(r * 255);
  const G = Math.round(g * 255);
  const B = Math.round(b * 255);
  return a >= 1 ? `rgb(${R},${G},${B})` : `rgba(${R},${G},${B},${a})`;
}

function paintAttrs(paint) {
  if (!paint) return 'fill="none" stroke="none"';
  // Absent hasFill / hasStroke default to whether the colour is present.
  const hasFill = paint.hasFill ?? paint.fillRgba != null;
  const hasStroke = paint.hasStroke ?? paint.strokeRgba != null;
  const fill = hasFill ? rgba(paint.fillRgba) : "none";
  const stroke = hasStroke ? rgba(paint.strokeRgba) : "none";
  const sw = hasStroke ? paint.strokeWidth ?? 1 : 0;
  return `fill="${fill}" stroke="${stroke}" stroke-width="${sw}"`;
}

// Vertices evenly spaced around a centre; radiusAt varies them for stars.
function radialPoints(cx, cy, count, radiusAt, rotationDegrees) {
  const start = (rotationDegrees * Math.PI) / 180;
  const out = [];
  for (let i = 0; i < count; i++) {
    const a = start + (i * 2 * Math.PI) / count;
    const r = radiusAt(i);
    const x = cx + Math.cos(a) * r;
    const y = cy + Math.sin(a) * r;
    expand(x, y);
    out.push(`${x},${y}`);
  }
  return out.join(" ");
}

const parts = [];
let minX = Infinity,
  minY = Infinity,
  maxX = -Infinity,
  maxY = -Infinity;

function expand(x, y) {
  minX = Math.min(minX, x);
  minY = Math.min(minY, y);
  maxX = Math.max(maxX, x);
  maxY = Math.max(maxY, y);
}

for (const e of doc.entities) {
  const c = comps(e);
  const [wx, wy] = worldPos(e.id);
  const paint = c["rig.paint.fill_stroke"];
  const attrs = paintAttrs(paint);
  const name = c["rig.meta.named"]?.name ?? e.id;

  const tag = `data-id="${e.id}" data-name="${name}"`;

  const rect = c["rig.geometry.rectangle"];
  if (rect) {
    const x = wx + rect.x;
    const y = wy + rect.y;
    expand(x, y);
    expand(x + rect.width, y + rect.height);
    const round = rect.cornerRadius ? ` rx="${rect.cornerRadius}"` : "";
    parts.push(
      `  <rect ${tag} x="${x}" y="${y}" width="${rect.width}" height="${rect.height}"${round} ${attrs} />`
    );
  }

  const ellipse = c["rig.geometry.ellipse"];
  if (ellipse) {
    const cx = wx + ellipse.cx;
    const cy = wy + ellipse.cy;
    expand(cx - ellipse.rx, cy - ellipse.ry);
    expand(cx + ellipse.rx, cy + ellipse.ry);
    parts.push(
      `  <ellipse ${tag} cx="${cx}" cy="${cy}" rx="${ellipse.rx}" ry="${ellipse.ry}" ${attrs} />`
    );
  }

  const line = c["rig.geometry.line"];
  if (line) {
    const x1 = wx + line.x1;
    const y1 = wy + line.y1;
    const x2 = wx + line.x2;
    const y2 = wy + line.y2;
    expand(x1, y1);
    expand(x2, y2);
    parts.push(`  <line ${tag} x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" ${attrs} />`);
  }

  const poly = c["rig.geometry.polygon"];
  if (poly) {
    const pts = poly.points.map(([px, py]) => {
      const x = wx + px;
      const y = wy + py;
      expand(x, y);
      return `${x},${y}`;
    });
    const el = poly.closed === false ? "polyline" : "polygon";
    parts.push(`  <${el} ${tag} points="${pts.join(" ")}" ${attrs} />`);
  }

  const ngon = c["rig.geometry.regular_polygon"];
  if (ngon) {
    const pts = radialPoints(
      wx + ngon.cx,
      wy + ngon.cy,
      ngon.sides,
      () => ngon.radius,
      ngon.rotationDegrees ?? 0
    );
    parts.push(`  <polygon ${tag} points="${pts}" ${attrs} />`);
  }

  const star = c["rig.geometry.star"];
  if (star) {
    const pts = radialPoints(
      wx + star.cx,
      wy + star.cy,
      star.points * 2,
      (i) => (i % 2 === 0 ? star.radius : star.innerRadius),
      star.rotationDegrees ?? 0
    );
    parts.push(`  <polygon ${tag} points="${pts}" ${attrs} />`);
  }

  const mesh = c["rig.geometry.mesh"];
  if (mesh?.positions) {
    const pts = [];
    for (let i = 0; i + 2 < mesh.positions.length; i += 3) {
      const x = wx + mesh.positions[i];
      const y = wy + mesh.positions[i + 1];
      pts.push(`${x},${y}`);
      expand(x, y);
    }
    // Use indices if present to order triangle vertices; for filled still, convex hull via indices
    let ordered = pts;
    if (mesh.indices?.length) {
      ordered = mesh.indices.map((i) => pts[i]).filter(Boolean);
    }
    parts.push(
      `  <polygon data-id="${e.id}" data-name="${name}" points="${ordered.join(" ")}" ${attrs} />`
    );
  }
}

const pad = 24;
const vbX = minX - pad;
const vbY = minY - pad;
const vbW = maxX - minX + pad * 2;
const vbH = maxY - minY + pad * 2;

const svg =
  `<?xml version="1.0" encoding="UTF-8"?>\n` +
  `<!-- Generated by tools/render-svg.mjs from examples/minimal-scene.json — do not edit by hand. -->\n` +
  `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${vbX} ${vbY} ${vbW} ${vbH}" role="img" aria-label="Minimal Rig scene">\n` +
  `  <rect x="${vbX}" y="${vbY}" width="${vbW}" height="${vbH}" fill="#0b0d10" />\n` +
  parts.join("\n") +
  `\n</svg>\n`;

const normalize = (t) => t.replace(/\r\n/g, "\n");

if (checkOnly) {
  const actual = fs.existsSync(out) ? normalize(fs.readFileSync(out, "utf8")) : null;
  if (actual !== normalize(svg)) {
    console.error("site/scene.svg is out of date — run: node tools/render-svg.mjs");
    process.exit(1);
  }
  console.log("scene.svg up to date");
} else {
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, svg);
  console.log("wrote", path.relative(root, out));
}
