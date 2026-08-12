## Summary

<!-- Why this change exists (1–3 bullets). -->

## Ten Commandments checklist

See [docs/contract/commandments.md](docs/contract/commandments.md).

- [ ] **Data before code** — POD / plain fields named (or N/A: no new entity meaning)
- [ ] **Pi floor** — compiles + smooth on Pi-class host, or **Pi risk** called out below
- [ ] **Rebuild-cheap** — change localized; or **rebuild-cost risk** called out below
- [ ] **Pack boundaries** — rigComponent stays data; systems/UI stay in code packs
- [ ] **Teaching API** — creators / SUDE helpers preferred over new `M*` ceremony for artists
- [ ] **Shoulders** — reused same-spirit lib / existing dep when fit; no NIH wheel, no wrong-spirit planet
- [ ] **Vocabulary** — pack / Contract / fulfillment / Host (no “addon”)
- [ ] **Gates** — `tools/check-invariants` + format + `contract_smoke` + example(s) touched
- [ ] **No fake stubs** — product paths finish or stay unpublished (no “not implemented yet” UI)

### Risk callouts (required when relevant)

- **Pi risk:** <!-- none / describe -->
- **Rebuild-cost risk:** <!-- none / describe -->

## Test plan

- [ ] `./tools/check-invariants.sh` (or `tools\check-invariants.bat`)
- [ ] Format + `contract_smoke` (see `skills/rigkit-build`)
- [ ] Built example(s) / pack example(s) touched
