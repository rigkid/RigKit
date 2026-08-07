#!/usr/bin/env node
/**
 * Inject golden entity snippets into docs from examples/*.json.
 *
 * SUDE sketches are deliberately different densities across README /
 * examples / skill — leave them alone. This tool only syncs marked JSON.
 *
 *   node tools/sync-snippets.mjs           # write
 *   node tools/sync-snippets.mjs --check   # exit 1 if stale
 *
 * Markers (valid in Markdown and HTML):
 *   <!-- rig:begin entity=demo-rect from=examples/minimal-scene.json -->
 *   ... replaced region ...
 *   <!-- rig:end -->
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const checkOnly = process.argv.includes("--check");

const TARGETS = [
  "README.md",
  "examples/README.md",
  "skills/generating-rig-documents/SKILL.md",
  "site/index.html",
];

const beginRe =
  /<!--\s*rig:begin\s+entity=([^\s]+)\s+from=([^\s]+)\s*-->/;
const blockRe =
  /<!--\s*rig:begin\s+entity=([^\s]+)\s+from=([^\s]+)\s*-->[\s\S]*?<!--\s*rig:end\s*-->/g;

function loadEntity(fromRel, entityId) {
  const doc = JSON.parse(fs.readFileSync(path.join(root, fromRel), "utf8"));
  const ent = (doc.entities || []).find((e) => e.id === entityId);
  if (!ent) {
    throw new Error(`entity "${entityId}" not found in ${fromRel}`);
  }
  return ent;
}

function formatMd(entity) {
  return (
    "```json\n" + JSON.stringify(entity, null, 2) + "\n```"
  );
}

function escapeHtml(s) {
  return s
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");
}

function formatHtml(entity) {
  return (
    '<pre class="code"><code>' +
    escapeHtml(JSON.stringify(entity, null, 2)) +
    "</code></pre>"
  );
}

function wrap(begin, body, isHtml) {
  // Preserve exact begin marker text from the file via reconstruction
  return `${begin}\n${body}\n<!-- rig:end -->`;
}

let stale = 0;
let written = 0;

for (const rel of TARGETS) {
  const file = path.join(root, rel);
  if (!fs.existsSync(file)) {
    console.error(`missing target: ${rel}`);
    process.exitCode = 1;
    continue;
  }
  const isHtml = rel.endsWith(".html");
  let text = fs.readFileSync(file, "utf8");
  const matches = [...text.matchAll(blockRe)];
  if (matches.length === 0) {
    console.error(`${rel}: no rig:begin markers`);
    process.exitCode = 1;
    continue;
  }

  let next = text;
  for (const m of matches) {
    const entityId = m[1];
    const fromRel = m[2];
    const beginTag = `<!-- rig:begin entity=${entityId} from=${fromRel} -->`;
    const entity = loadEntity(fromRel, entityId);
    const body = isHtml ? formatHtml(entity) : formatMd(entity);
    const replacement = wrap(beginTag, body, isHtml);
    next = next.replace(m[0], replacement);
  }

  if (next === text) {
    console.log(`ok  ${rel}`);
  } else if (checkOnly) {
    console.error(`stale ${rel}`);
    stale++;
  } else {
    fs.writeFileSync(file, next);
    console.log(`wrote ${rel}`);
    written++;
  }
}

if (checkOnly && stale) {
  console.error(`${stale} stale snippet file(s) — run: node tools/sync-snippets.mjs`);
  process.exit(1);
}

if (!checkOnly) {
  console.log(`snippets synced — ${written} file(s) written`);
} else {
  console.log("snippets ok");
}
