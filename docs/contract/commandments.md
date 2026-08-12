# The RigKit Ten Commandments

Constitution for humans and agents. **AI collaboration is invited** — same rules, same gates, same vocabulary. Commentary lives in [AGENTS.md](../../AGENTS.md) and the task skills. **Do not grow this file into a wiki.**

1. **Data before code.** Name the POD fields first. No panel, system, or helper until the entity meaning is plain data.
2. **Pi is the floor.** If it does not compile and run smooth on Raspberry Pi, it is not the product.
3. **Rebuilds stay cheap.** Thin core, pack weight, narrow includes. A coffee-break compile is a design bug.
4. **rigComponent = data. rigSystems = code.** Never blur that for convenience. Behavior packs read data; they do not become the data.
5. **Creators teach; managers serve packs.** Artists get `makeRect` / `setup`–`update`–`draw`. `M*` stays for internals.
6. **One vocabulary.** Pack ≠ addon. **RigWorks** (**Rig** in running text) = zero-code framework (grammar + entity/component POD + schemas). **RigKit** = coded Rig host — **floor is SUDE + ECS**. Contract/framework ≠ fulfillment. Host ≠ distribution. Same words everywhere — kill synonyms.
7. **Stand on shoulders.** Prefer an existing library over writing our own — only when it shares our spirit (Pi-cheap, data-friendly, rebuild-cheap, artist-warm), does what we need, and we can change it. Do not reinvent the wheel. Do not invent Mars (a wrong-spirit megastack or a parallel planet just to own every line).
8. **Seams stay small.** Extend through packs, `IApp`, `IMui`, POD + register. No second scene graph. No ceremony trees.
9. **No UI toolkit in `src/`.** UI through `IMui`; **rigImGui** is a fulfillment, not Rig.
10. **Gates over gospel.** Format, contract smoke, and the example you touched must pass before you call it done. Load-bearing rules get CI — not essays. Refuse the fog: explicit non-goals, next-year docs only, no archaeology, no hallucinated scope. **No fake stubs:** never ship a product path that pretends to work (no-op UI, empty remote, “not implemented yet” as Generate). Reserving POD / enum surface for a known port is fine — fulfillments and Kit UI expose only what runs.

## Not for this

RigKit is not a general web SaaS stack, not a desktop-only GPU playground, and not a place to teach manager ceremony as the artist path. Product apps live out of tree. Weight lands in packs. Prefer same-spirit libraries we can fork or patch; reject NIH wheels and alien planets alike.

## Enforce

| Layer | What |
|-------|------|
| Agents | This list + [AGENTS.md](../../AGENTS.md) + `skills/` |
| CI / local | `tools/check-invariants.sh` (+ format, `contract_smoke`, example builds) |
| PR | `.github/pull_request_template.md` checklist |

Pi risk and rebuild-cost risk: call them out in the PR when the change is heavy, widens hot headers, or is untested on arm64/GLES.

## AI collaboration

We invite agents to build with us. Load [AGENTS.md](../../AGENTS.md), obey the ten, run the gates. Humans review under the same commandments — no special AI track, no fog.
