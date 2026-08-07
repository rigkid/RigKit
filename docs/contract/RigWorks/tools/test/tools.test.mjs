import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");

function run(script, args = [], cwd = root) {
  return spawnSync(process.execPath, [path.join(root, script), ...args], {
    encoding: "utf8",
    cwd,
  });
}

describe("check-links", () => {
  it("fails on a broken markdown link", () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), "rig-links-md-"));
    fs.writeFileSync(path.join(dir, "doc.md"), "[x](./missing.md)\n");
    // check-links walks from repo root; instead spawn a tiny wrapper by
    // copying the fixture tree under a temp that mimics structure is hard.
    // Use the committed fixture via chdir into fixtures/links and patch:
    // The tool always walks from repo root, so inject a temp broken file
    // is invasive. Instead, run against a copied isolated mini-repo.
    const mini = path.join(dir, "repo");
    fs.mkdirSync(path.join(mini, "tools"), { recursive: true });
    fs.copyFileSync(
      path.join(root, "tools/check-links.mjs"),
      path.join(mini, "tools/check-links.mjs")
    );
    fs.mkdirSync(path.join(mini, "docs"), { recursive: true });
    fs.writeFileSync(path.join(mini, "docs/a.md"), "[bad](./nope.md)\n");
    const r = spawnSync(process.execPath, ["tools/check-links.mjs"], {
      encoding: "utf8",
      cwd: mini,
    });
    assert.equal(r.status, 1);
    assert.match(r.stderr + r.stdout, /broken link/);
  });

  it("fails on a broken HTML href", () => {
    const dir = fs.mkdtempSync(path.join(os.tmpdir(), "rig-links-html-"));
    const mini = path.join(dir, "repo");
    fs.mkdirSync(path.join(mini, "tools"), { recursive: true });
    fs.copyFileSync(
      path.join(root, "tools/check-links.mjs"),
      path.join(mini, "tools/check-links.mjs")
    );
    fs.mkdirSync(path.join(mini, "site"), { recursive: true });
    fs.writeFileSync(
      path.join(mini, "site/index.html"),
      '<a href="./gone.html">x</a>\n'
    );
    const r = spawnSync(process.execPath, ["tools/check-links.mjs"], {
      encoding: "utf8",
      cwd: mini,
    });
    assert.equal(r.status, 1);
    assert.match(r.stderr + r.stdout, /broken link/);
  });
});

describe("smoke", () => {
  it("gen-schemas --check exits 0", () => {
    const r = run("tools/gen-schemas.mjs", ["--check"]);
    assert.equal(r.status, 0, r.stderr + r.stdout);
  });

  it("check-schema-parity exits 0", () => {
    const r = run("tools/check-schema-parity/check.mjs");
    assert.equal(r.status, 0, r.stderr + r.stdout);
  });

  it("sync-snippets --check exits 0", () => {
    const r = run("tools/sync-snippets.mjs", ["--check"]);
    assert.equal(r.status, 0, r.stderr + r.stdout);
  });

  it("render-svg --check exits 0", () => {
    const r = run("tools/render-svg.mjs", ["--check"]);
    assert.equal(r.status, 0, r.stderr + r.stdout);
  });
});
