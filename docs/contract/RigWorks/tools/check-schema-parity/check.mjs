#!/usr/bin/env node
/**
 * Fail if markdown catalog ids and schemas/json/*.schema.json drift apart.
 *   node tools/check-schema-parity/check.mjs
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const schemasRoot = path.join(root, "schemas");
const jsonDir = path.join(schemasRoot, "json");

function walkMd(dir, out = []) {
  for (const name of fs.readdirSync(dir)) {
    const p = path.join(dir, name);
    const st = fs.statSync(p);
    if (st.isDirectory()) {
      if (name === "json") continue;
      walkMd(p, out);
    } else if (name.endsWith(".md") && name !== "README.md") {
      out.push(p);
    }
  }
  return out;
}

const idRe = /^#\s+`?(rig\.[a-z0-9_.]+)`?\s*$/m;
const mdIds = new Set();
for (const file of walkMd(schemasRoot)) {
  const text = fs.readFileSync(file, "utf8");
  const m = text.match(idRe);
  if (!m) {
    console.error(`no schema id heading in ${path.relative(root, file)}`);
    process.exitCode = 1;
    continue;
  }
  mdIds.add(m[1]);
}

const jsonIds = new Set();
for (const name of fs.readdirSync(jsonDir)) {
  if (!name.endsWith(".schema.json")) continue;
  if (name === "_defs.schema.json") continue;
  jsonIds.add(name.replace(/\.schema\.json$/, ""));
}

const missingJson = [...mdIds].filter((id) => !jsonIds.has(id)).sort();
const missingMd = [...jsonIds].filter((id) => !mdIds.has(id)).sort();

if (missingJson.length) {
  console.error("Markdown schemas missing JSON Schema:");
  for (const id of missingJson) console.error("  ", id);
  process.exitCode = 1;
}
if (missingMd.length) {
  console.error("JSON Schema files missing markdown:");
  for (const id of missingMd) console.error("  ", id);
  process.exitCode = 1;
}

if (!process.exitCode) {
  console.log(`parity ok — ${mdIds.size} schemas`);
}
