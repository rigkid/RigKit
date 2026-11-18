# Gate-Driven Development

**A short field guide for people who like green builds more than green essays.**

Also called **GDD** here. Elsewhere, GDD often means “Game Design Document.” If someone asks which one you mean, say the long form. This book is about the gates.

RigKit already practices this under Commandment 10 — *Gates over gospel.* This guide names the habit, teaches it, and shows the code shape so humans and agents share the same punchline: **done means the machines that can say no are quiet.**

---

## How to read this

Chapters are short on purpose. Skim 1–3 for the idea. Steal recipes from 7–9 when you ship. The code is RigKit-shaped (C++20, doctest, `--smoke` flags) but the method travels.

What this book does **not** do:

- Replace [commandments.md](contract/commandments.md) or [AGENTS.md](../AGENTS.md)
- Demand org-wide test-first theater
- Promise that gates invent product taste

Taste still needs a human. Gates keep the floor from rotting while you argue about taste.

---

## 1. The night gospel failed

Imagine two teams ship the same bug: a property pointer that no longer lands inside the component.

Team Gospel writes a three-page ADR: “Reflection shall remain valid.” Everyone nods. Nobody runs anything. Next week the bug is still there, now with a citation.

Team Gate adds twelve lines:

```cpp
TEST_CASE("GetProperties pointers land in component storage") {
	SpineFixture f;
	auto& ecs = f.ecs();
	auto entity = ecs.createEntity();
	auto& comp = ecs.addComponent<ecs::CTransform>(entity);
	const auto* info = f.findType("Transform");
	REQUIRE(info != nullptr);

	auto props = ecs.registeredProperties(*info, entity);
	REQUIRE_FALSE(props.empty());
	for (const auto& p : props) {
		CHECK(propPointsIntoComponent(p, &comp, sizeof(ecs::CTransform)));
	}
}
```

CI goes red. The author fixes the reflection. The ADR is optional. The gate is not.

That is Gate-Driven Development: **proof at the seam beats belief about the seam.**

---

## 2. What a gate is

A **gate** is a check that can fail you without a meeting.

| Gate | Question it answers |
|------|---------------------|
| Format / invariants | “Does this still look like our tree?” |
| `contract_smoke` | “Did we break the data layer / spine packs?” |
| Example `--smoke` | “Does the path I touched still boot and assert?” |
| Pi compile / run | “Is this still the product floor?” |

A gate has four traits:

1. **Automatic** — a human should not have to “just remember.”
2. **Fast enough** — if it needs a coffee, it is a design bug (or a rare nightly).
3. **Mean** — red means stop; green is not theater.
4. **Owned** — someone knows what it guards and when to extend it.

Essays are fine as maps. They are terrible as locks. Locks belong in machines.

---

## 3. Gospel, TDD, and GDD

Three ways teams say “quality”:

| Style | Motto | Done means |
|-------|-------|------------|
| **Gospel-driven** | Write it down | The doc matches the dream |
| **Test-driven (TDD)** | Red, green, refactor | A failing unit test led the change |
| **Gate-driven (GDD)** | Prove the seams | Touched gates are green |

GDD is not anti-test. It is anti-**ceremony-as-substitute-for-proof**.

TDD is a sharp knife for pure logic: codecs, math, parsers, schema round-trips. Use it there happily.

GDD is the kitchen rule for a host like RigKit: SUDE loop, packs, GLES, Pi, artist helpers. Mocking `IMui` until the panel “passes” often proves only that your mock is lonely.

**Rule of thumb:** if the bug would only show with a real fixture, real register, or real windowless boot — write a smoke gate, not a puppet theater.

---

## 4. Anatomy of a good gate

### Bad gate (theater)

```cpp
TEST_CASE("osc somehow works") {
	CHECK(true); // "we'll fill this in later"
}
```

Green forever. Teaches nothing. Worse than no gate: it lies with confidence.

### Better gate (behavioral smoke)

`oscHost` does not pretend. It binds, loopbacks, and leaves:

```cpp
bool OscHost::runOscSmoke() {
	if (!m_osc) {
		spdlog::error("smoke-osc: rigOsc not registered");
		return false;
	}
	return m_osc->smokeLoopback();
}

// in setup(), after packs boot:
if (m_smokeOsc) {
	const bool ok = runOscSmoke();
	if (!ok) {
		spdlog::error("oscHost --smoke-osc failed: {}",
					  m_osc ? m_osc->lastError() : "no pack");
		std::exit(1);
	}
	spdlog::info("oscHost --smoke-osc OK");
	// close the window / exit the host
}
```

Run it:

```bash
./examples/oscHost/build/bin/oscHost --smoke-osc
```

Exit code nonzero = gate slammed. That is the whole romance.

### Best gate (names the invariant)

Headless contract smoke guards portable types and reflection without opening a window:

```bash
cmake -S . -B build -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke
```

Inside, cases read like contracts, not vibes:

```cpp
TEST_CASE("rigComponent registers portable spine types") {
	SpineFixture f;
	requireType(f, "Transform", true);
	requireType(f, "Rectangle", true);
	requireType(f, "Mesh", true);
	requireType(f, "UiPanel", true);
	// ...
}
```

When someone “cleverly” demotes a portable type, the gate does not care about their cleverness.

---

## 5. The ladder (cheap to mean)

Climb only as high as the change needs. Stop when the rung holds.

```text
1. Static        check-invariants, clang-format
2. Contract      contract_smoke (data, properties, purity)
3. Pack / app    example --smoke-X, fixture .rig round-trip
4. Floor         Pi build + run (or explicit Pi-risk callout)
```

### Example: you changed a POD field

Stay on rungs 1–2. Add or extend a doctest that round-trips the field. Do not open ImGui.

```cpp
TEST_CASE(".rig round-trip keeps star points") {
	SpineFixture f;
	auto e = f.ecs().createEntity();
	auto& star = f.ecs().addComponent<ecs::CStar>(e);
	star.points = 7;
	// serialize -> deserialize into a fresh world
	// REQUIRE(loaded.points == 7);
}
```

### Example: you changed OSC bind behavior

Rung 3. Keep `oscHost --smoke-osc` green. If the new behavior is load-bearing, extend `smokeLoopback()` — do not write a blog post in the README and call it coverage.

### Example: you added a desktop-only GPU path

Rung 4 fires even if desktop is green. Either the Pi path still works, or the PR says **Pi risk** out loud. Silence is not a strategy.

---

## 6. A day in GDD (the loop)

Forget the vision board. Here is Tuesday:

1. **Name the data** (Commandment 1). Fields first.
2. **Smallest change** that teaches the machine the new truth.
3. **Run the gates that own that surface.**
4. If a bug class has no gate, **promote the bug into a gate** before you merge the fix alone.

Local CI before commit is not optional politeness. It is the sport:

```bash
# static
./tools/check-invariants.sh   # on Windows, run from Git Bash

# contract
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke

# the example you touched
cmake --build examples/oscHost/build --target oscHost
./examples/oscHost/build/bin/oscHost --smoke-osc
```

If GitHub CI is where you learn you broke `main`, you are doing archaeology in public.

---

## 7. Cookbook: six recipes with code

### Recipe A — Fixture smoke for a document path

Ship a tiny `.rig`, open it headless, assert one invariant, exit.

```bash
./examples/example-stage/build/bin/example-stage \
  --open=data/scenes/basic-scene.rig --smoke
```

Pattern in the app:

```cpp
if (args.hasFlag("smoke")) {
	const bool ok = runFeatureSmoke(/* steps */ 120);
	spdlog::info(ok ? "example-stage --smoke ok"
					: "example-stage --smoke failed");
	if (!ok)
		std::exit(1);
	requestClose();
}
```

**Why it works:** real document, real systems, bounded steps. Artists can break it; CI can catch it.

### Recipe B — Pack smoke flag

Every code pack that can boot should grow a `--smoke-<pack>` (or pack example smoke) before it graduates from teething.

```cpp
// sketch: pack example main
if (cli.hasFlag("smoke-image")) {
	Raster r;
	REQUIRE_LOAD(decode("data/fixtures/dot.png", r));
	REQUIRE(r.width == 1 && r.height == 1);
	return 0;
}
```

Keep the fixture tiny. Pi does not need your 8K hero shot in CI.

### Recipe C — Pure helper: tiny TDD inside GDD

Here TDD is welcome. Red first:

```cpp
TEST_CASE("grey value of mid grey is ~0.5") {
	CHECK(rig::colorspace::greyValue({0.5f, 0.5f, 0.5f})
		  == doctest::Approx(0.5f).epsilon(0.01));
}
```

Implement until green. That unit test **is** a gate. You did not need a host.

### Recipe D — Boundary gate (data pack stays data)

```cpp
TEST_CASE("rigComponent headers do not pull systems") {
	// contract_smoke purity lane: scanning includes / compile probes
	CHECK(headerIsDataOnly("CTransform.h"));
}
```

When someone “just adds one helper” into a data pack, the purity gate is the adult in the room.

### Recipe E — Promote a regression

Bug: prefab spawn left a dangling selection id.

Wrong ending: fix the spawn, merge, hope.

GDD ending: fix the spawn **and** lock the door:

```cpp
TEST_CASE("destroy clears selection of dead entity") {
	auto world = makePrefabWorld();
	auto id = spawnBall(world);
	select(world, id);
	destroy(world, id);
	CHECK_FALSE(selectionContains(world, id));
}
```

Same bug cannot ghost you twice without buying a red CI.

### Recipe F — Explicit non-goal as a gate comment

Gates refuse fog. If something is out of scope, say so next to the check:

```cpp
TEST_CASE("contract_smoke stays windowless") {
	// Non-goal: ImGui panel layout. That lives in pack/example smokes.
	CHECK_FALSE(smokeOpensWindow());
}
```

Future-you will try to “help” by attaching a UI toolkit. The comment and the check form a buddy comedy.

---

## 8. When to add a gate (and when not to)

**Add a gate when:**

- The same bug class bit you twice
- A commandment is load-bearing (data vs code, Pi floor, no fake stubs)
- A pack is leaving teething / going public
- A serializer or schema field can silently drop data

**Do not add a gate when:**

- You are still sketching artist API feel in `draw()`
- The check would need a GPU farm and a priest
- You are encoding taste (“button should feel snappier”)
- You would only assert `CHECK(true)`

Lazy is efficient, not careless. One mean smoke beats twenty polite ones.

---

## 9. How gates die (keep them mean)

Gates rot in predictable ways. Spot the smell early.

| Smell | What happened | Fix |
|-------|---------------|-----|
| Always green | Asserts nothing real | Delete or rewrite with a fixture |
| Always flaky | Timing / network / cwd | Bound steps; fix paths to exe-relative data |
| Slow coffee | Too much world in one binary | Split unit smoke vs integration smoke |
| README gospel | “Remember to run X” | Put X in CI / local-CI skill |
| Mock mirror | Tests pass, product fails | Prefer `--smoke` over puppet hosts |

**Golden rule:** if developers learn to ignore a gate, it is already gone. Delete it or make it honest.

---

## 10. GDD for agents (and humans who type like agents)

Same commandments, same gates. No special AI track.

Agents especially love gospel: long plans, confident stubs, “not implemented yet” UI. GDD is the antidote:

- Do not claim done until local gates for the touched surface are green
- Prefer extending `contract_smoke` or a `--smoke` over a new markdown commandment
- No fake stubs on product paths — a red gate is kinder than a green lie

If you are an agent reading this: the book is not homework. The book is why `tools/check-invariants` and `contract_smoke` get to ruin your afternoon before a reviewer does.

---

## 11. Pocket card

Print this. Tape it to the Pi.

```text
GDD pocket card
---------------
1. Name the POD fields.
2. Smallest change.
3. Run gates that own the surface.
4. Bug twice? Promote it to a gate.
5. Pure logic? Tiny TDD is fine.
6. UI / GLES / SUDE? Smoke the real path.
7. Pi is the floor. Say Pi risk if untested.
8. Few mean gates > many polite ones.
9. Gospel maps; gates lock.
10. Green locally, then commit.
```

---

## 12. Epilogue: fun is a feature

Gate-Driven Development is allowed to be fun. Red CI is a plot twist, not a moral failing. A twelve-line smoke that catches a reflection bug is comedy gold: the universe tried to be clever; you were lazier and correct.

Ship by proof at the seams you care about. Keep essays thin. Keep rebuilds cheap. Keep the Pi in the story.

When someone asks whether you are “doing TDD now,” smile and say:

> Sometimes for codecs. Always for gates.

Then run the smoke.

---

## See also

- [commandments.md](contract/commandments.md) — especially #10 Gates over gospel
- [AGENTS.md](../AGENTS.md) — playbook humans and agents share
- [contributing.md](contributing.md) — local CI, contract smoke
- [skills/rigkit-build/SKILL.md](../skills/rigkit-build/SKILL.md) — build + verify loop
- [tools/contract_smoke/README.md](../tools/contract_smoke/README.md) — headless doctest lane
- [contract/pi-host.md](contract/pi-host.md) — the floor that keeps us honest
