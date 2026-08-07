#!/usr/bin/env node
/**
 * Validate Rig JSON documents against schemas/json/.
 *
 *   node tools/rig-validate/cli.js path/to/doc.json
 *   node tools/rig-validate/cli.js --strict examples/*.json
 *
 * Envelope, per-component fields, duplicate ids, and dangling entity refs.
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import Ajv2020 from "ajv/dist/2020.js";
import addFormats from "ajv-formats";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "../..");
const schemaDir = path.join(repoRoot, "schemas", "json");

const args = process.argv.slice(2);
const strict = args.includes("--strict");
const files = args.filter((a) => a !== "--strict" && !a.startsWith("-"));

if (files.length === 0) {
  console.error("Usage: rig-validate [--strict] <file.json>...");
  process.exit(2);
}

function loadJson(file) {
  return JSON.parse(fs.readFileSync(file, "utf8"));
}

/** Raw schema JSON by component id, for entity-reference walking. */
const rawSchemas = new Map();

function loadAllSchemas(ajv) {
  const names = fs.readdirSync(schemaDir).filter((n) => n.endsWith(".schema.json"));
  for (const name of names) {
    const schema = loadJson(path.join(schemaDir, name));
    ajv.addSchema(schema);
    rawSchemas.set(name.replace(/\.schema\.json$/, ""), schema);
  }
  return names;
}

const ENTITY_REF = "./_defs.schema.json#/$defs/entity";

/**
 * Walk a schema and an instance together, collecting every entity-typed value.
 * Handles nested objects, arrays, and tagged unions.
 */
function collectEntityRefs(schema, value, pointer, out) {
  if (!schema || typeof schema !== "object" || value === undefined) return;

  if (schema.$ref === ENTITY_REF) {
    if (typeof value === "string") out.push({ pointer, id: value });
    return;
  }

  for (const branch of schema.oneOf ?? schema.anyOf ?? schema.allOf ?? []) {
    collectEntityRefs(branch, value, pointer, out);
  }

  if (schema.properties && value && typeof value === "object" && !Array.isArray(value)) {
    for (const [key, sub] of Object.entries(schema.properties)) {
      collectEntityRefs(sub, value[key], `${pointer}/${key}`, out);
    }
  }

  if (schema.items && Array.isArray(value)) {
    value.forEach((item, i) => collectEntityRefs(schema.items, item, `${pointer}/${i}`, out));
  }
}

function repoVersion() {
  try {
    return fs.readFileSync(path.join(repoRoot, "VERSION"), "utf8").trim();
  } catch {
    return null;
  }
}

function isNewerThan(a, b) {
  const pa = a.split(".").map(Number);
  const pb = b.split(".").map(Number);
  for (let i = 0; i < 3; i++) {
    if ((pa[i] ?? 0) !== (pb[i] ?? 0)) return (pa[i] ?? 0) > (pb[i] ?? 0);
  }
  return false;
}

const ajv = new Ajv2020({
  allErrors: true,
  strict: false,
  validateSchema: false,
});
addFormats(ajv);
loadAllSchemas(ajv);

const validateDoc = ajv.getSchema("https://rig.works/schemas/rig.document.schema.json");
if (!validateDoc) {
  console.error("Failed to load rig.document schema");
  process.exit(1);
}

let failed = false;

for (const file of files) {
  const abs = path.resolve(file);
  let data;
  try {
    data = loadJson(abs);
  } catch (err) {
    console.error(`${file}: cannot parse JSON — ${err.message}`);
    failed = true;
    continue;
  }

  const ok = validateDoc(data);
  const errors = [];

  if (!ok) {
    for (const e of validateDoc.errors ?? []) {
      errors.push({
        level: "error",
        path: e.instancePath || "/",
        message: `${e.message}${e.params ? " " + JSON.stringify(e.params) : ""}`,
      });
    }
  }

  const version = repoVersion();
  if (version && typeof data?.rig === "string" && isNewerThan(data.rig, version)) {
    errors.push({
      level: "warn",
      path: "/rig",
      message: `document targets ${data.rig}, but this catalog is ${version}`,
    });
  }

  if (Array.isArray(data?.entities)) {
    // Entity id uniqueness
    const ids = new Set();
    for (const ent of data.entities) {
      if (!ent?.id) continue;
      if (ids.has(ent.id)) {
        errors.push({
          level: "error",
          path: `/entities`,
          message: `duplicate entity id "${ent.id}"`,
        });
      }
      ids.add(ent.id);
    }

    // Per-component validation (catches unknown ids + deep field errors)
    const refs = [];
    data.entities.forEach((ent, ei) => {
      const comps = ent?.components;
      if (!comps || typeof comps !== "object") return;
      for (const [schemaId, payload] of Object.entries(comps)) {
        const base = `/entities/${ei}/components/${schemaId}`;

        // x.<vendor>.<name> is a host component the Contract has not named.
        // There is no schema to check it against, so report it and move on —
        // reporting is the point, since none of it is portable.
        if (schemaId.startsWith("x.")) {
          errors.push({
            level: "note",
            path: base,
            message: `extension component "${schemaId}" — carried, not portable`,
          });
          continue;
        }

        const schemaUri = `https://rig.works/schemas/${schemaId}.schema.json`;
        const validateComp = ajv.getSchema(schemaUri);
        if (!validateComp) {
          errors.push({
            level: strict ? "error" : "warn",
            path: base,
            message: `unknown schema id "${schemaId}"`,
          });
          continue;
        }
        if (!validateComp(payload)) {
          for (const e of validateComp.errors ?? []) {
            errors.push({
              level: "error",
              path: `${base}${e.instancePath || ""}`,
              message: e.message,
            });
          }
        }
        collectEntityRefs(rawSchemas.get(schemaId), payload, base, refs);
      }
    });

    // Dangling entity references (null means "none" and is valid)
    for (const { pointer, id } of refs) {
      if (!ids.has(id)) {
        errors.push({
          level: "error",
          path: pointer,
          message: `entity reference "${id}" matches no entity id in this document`,
        });
      }
    }
  }

  const hard = errors.filter((e) => e.level === "error");
  const soft = errors.filter((e) => e.level === "warn");

  // Notes report extension components. They are listed but never change the
  // verdict, so a host carrying its own components still validates clean.
  if (hard.length) failed = true;
  console.log(`${hard.length ? "FAIL" : soft.length ? "WARN" : "ok  "} ${file}`);
  for (const e of errors) {
    console.log(`  [${e.level}] ${e.path}: ${e.message}`);
  }
}

process.exit(failed ? 1 : 0);
