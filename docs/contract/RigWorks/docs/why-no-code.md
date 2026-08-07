# Why no code

**A no-code creative application framework. Because no code is the best code.**

## What "no code" means here

The phrase is usually sold as "you will never program again". Rig does not claim that. It claims something narrower and more durable: **the framework itself contains no code.**

| Reading | True of Rig? |
|---------|-----------------|
| The framework ships zero code — nothing to link, embed, compile, or upgrade. | **Yes.** This is the entire design. |
| Content is authored as data, not source. A document is the artifact. | **Yes.** [`rig.document`](../schemas/document.md) travels; behavior stays in the host. |
| Nobody writes code anywhere. | **No.** Someone writes the host. Rig is not that someone. |

Rig is a framework you never install. It frames how a creative app is shaped — [a loop](sude.md), [composed data](ecs.md), [named field layouts](../schemas/) — and then gets out of the way. Adopting it is a decision, not a dependency.

## Because no code is the best code

Every line you ship is a liability with a longer life than its usefulness. Code that does not exist has properties no well-written library can match:

| It cannot | Because |
|-----------|---------|
| Have a bug | There is no implementation to be wrong |
| Have a CVE | There is no supply chain to audit |
| Break on upgrade | There is nothing to upgrade |
| Force a migration | Field names do not need porting |
| Lose its maintainer | A written rule has no bus factor |
| Fight your build | No headers, flags, ABI, or transitive dependencies |
| Lock your language | Any language can emit JSON and call four functions |

A library asks you to trust a team's release cadence forever. A contract asks you to read a page once.

## Not the same as a visual programming app

TouchDesigner, Max/MSP, vvvv, and Cables are often called "no-code," but they **are code**: Python, GLSL, expressions, and node ABIs under a GUI. Their project formats (`.toe`, `.v4p`, …) are private. Rig's "no code" means the *framework* ships none — not that authoring never involves programming. Those tools are excellent fulfillments; Rig is what two of them would need to speak to exchange work.

## Railway gauge

Independent rail companies once picked different track widths. Their trains physically could not run on each other's lines until they agreed on a number. Creative tools have the same problem with scenes, sequences, and graphs. Converters rot; agreement lasts. Rig is the gauge — field names and units — not another locomotive.

## What a library would have cost

Had Rig shipped as `librig`, the cost would land on exactly the people it is meant to serve:

- **Language lock.** A C++ library excludes the Python sketch, the browser toy, the Arduino sign.
- **Version lock.** Your app's floor becomes the library's floor. Two packs on two versions cannot load together.
- **Platform reach.** A microcontroller cannot take the dependency, so embedded work forks or opts out.
- **Fork pressure.** The moment someone needs a behavior the library forbids, they vendor it — and the shared meaning quietly diverges.
- **Coupling of unrelated things.** A renderer bug fix and a schema addition arrive in the same release.

Data has none of these. A `rig.spatial.transform` payload written by a Rust CLI loads in a C++ host and an LED controller, none of which know the others exist.

## Concept is more important than execution

**Concept** is the [Contract](terms.md): what a transform *is*, which units it uses, that rotation is a quaternion in `x, y, z, w` order. **Execution** is the [fulfillment](terms.md): the code that draws it this year, on this GPU, in this language.

Executions have a half-life. Renderers get rewritten, GPUs are replaced, frameworks are abandoned, languages fall out of fashion. Concepts do not decay:

```json
"rig.music.transport": { "bpm": 128, "timeSigNum": 4, "timeSigDen": 4 }
```

Whether that drives a C++ audio thread, a web worker, or a strip of LEDs is execution, and it will be rewritten several times. `"bpm": 128` will still mean 128. Every document ever written against the concept survives every rewrite of every execution.

This is why Rig invests its effort where it does. The repository contains schemas, units, and rules — the parts that are expensive to agree on and cheap to keep. It contains no renderer, because renderers are the easy, disposable part.

It is also why "ship what you support" is not a compromise. A host that implements six schemas and ignores sixty-three is fully Rig. Partial execution of a shared concept still interoperates; a partial library does not link.

## The honest price

A data framework moves work rather than deleting it. Be clear about what you take on:

| Cost | Detail |
|------|--------|
| You implement the loop | No `rig.h` to include. [SUDE](sude.md) is four hooks; you write them. |
| No compiler enforcement | Nothing fails to build when you drift. [`rig-validate`](../tools/rig-validate/) is the enforcement, so run it. |
| Agreement is social | A schema only pays off once a second host speaks it. Ids are cheap; consensus is not. |
| Underspecification bites | "Colour" without a range, "angle" without units — the Contract is only as good as its prose. |

## When a library is the better answer

Take a library when you want one implementation, in one language, on one platform, shipping this week. Rig pays off under different conditions:

- You expect **more than one host** — a desktop tool and an embedded show, an editor and a renderer.
- You expect the work to **outlive its implementation** — installations, archives, long-running instruments.
- You want **tools and models to author content** without linking anything ([AI co-coding](ai-collaboration.md)).
- You need to **subset** — an LED controller taking transforms and colours, and nothing else.

## Next

[terms.md](terms.md) for the vocabulary · [honors.md](honors.md) for the minimum bar · [ai-collaboration.md](ai-collaboration.md) for why this matters more every year
