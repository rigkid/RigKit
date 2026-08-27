---
name: rigkit-git-safety
description: >-
  Secrets, PII, and copyright checklist for RigKit git commits and pushes. Use
  before staging, committing, or pushing changes, when reviewing a diff or new
  file for anything secret-shaped or unlicensed, or when the user mentions
  credentials, API keys, tokens, PII, GDPR data, test fixtures, copyrighted
  assets/code, or cleaning secrets out of git history.
---

# git safety

## Secret, Risk & Copyright (SRC) list

Three tiers. Tier decides the rule - do not judge by file extension alone.

| Tier | Meaning | Rule |
|------|---------|------|
| **SECRET** | The content itself is the sensitive thing | Never commit. No exceptions for "test-looking", hashed, or "internal only". |
| **RISK** | Commonly *carries* secrets/PII without being secret by definition | Inspect content before committing. Extension/filetype is not a reliable signal - a `.log`, `.db`, or `.key` file can be empty of anything sensitive, or full of it. |
| **COPYRIGHT** | Legal/licensing risk, not a privacy risk - someone else's IP without checked rights | Never commit without a verified license. Same action as SECRET (don't commit), different consequence (DMCA/license violation, not exposure). |

Git history is forever: a later `.gitignore` entry or `git rm` does nothing retroactively once pushed - removal needs `git filter-repo` / BFG + force-push. Catch it before the first push, not after.

## SECRET - never commit

**Authentication artifacts beyond raw credentials**
- Session tokens, JWT/refresh tokens, cookies with session IDs
- OAuth client secrets, 2FA backup/recovery codes
- Password hashes (even hashed - shouldn't be committed either)
- Security questions & answers

**GDPR-style "special category" personal data** (extra-sensitive, not just "identifying")
- Racial/ethnic origin, religious/political beliefs, trade union membership
- Sexual orientation, health data, genetic/biometric data
- Criminal record info, immigration/visa status
- Children's data specifically (COPPA-type - treat more conservatively than adult PII)

**Business-sensitive, not personal but still shouldn't be committed**
- Unreleased pricing, contracts, NDAs, legal correspondence
- Internal infrastructure details: internal IPs, VPN configs, internal hostnames/network topology (a security risk even without any "person" attached)
- Business partner/client lists with commercial terms

**Financial/tax documents**
- W-2s, 1099s, tax returns, insurance policy numbers, pay stubs

**Location/travel records tied to a person**
- Calendar exports, travel itineraries with booking/passport numbers, vehicle plate/VIN tied to an owner

## RISK - inspect before committing

**Metadata that leaks personal info silently** (the sneaky category - nothing in the visible content looks like a secret, but the file carries it anyway)
- EXIF data in images: GPS coordinates of where a photo was taken, camera/device serial number, photographer name
- Document metadata: Word/PDF "Author" field, revision history embedding real names
- Some commercial font `.ttf`/`.otf` files bake the licensee's name/email into font metadata - relevant here since RigKit ships fonts (Roboto is fine/OSS, but flag it if anyone ever adds a paid font)

**"Real data used as test fixtures"** - a classic real-world leak
- A dev copies a slice of a production DB, customer export, or real bug-report email thread into a test fixture "just to get realistic data," and it goes in as `test_data.json`/`sample.csv`
- This is often the actual way PII ends up in a git history - not a deliberate credentials file, but "realistic-looking" fixture data that turns out to be real

**Logs, databases, caches, exports**
- Log files aren't secret by design, but debug logging routinely captures tokens, paths-with-usernames, and user input by accident
- `.db`/`.sqlite`/`.key`-shaped files aren't automatically secret either - the content decides, not the extension

## COPYRIGHT - never commit without verified rights

**Assets** (images, audio, fonts, video) not cleared for redistribution
- Stock photos, artwork, icons, or screenshots of other commercial software pasted into docs/examples/tests "just to show the idea"
- Paid/commercial fonts or sound effects dropped into `assets/` without a redistribution license - see `packs/rigImGui/fonts/LICENSE.fonts.txt` for how RigKit already tracks font licensing (Roboto/OSS is fine; a paid font is not, even if it renders correctly)
- Company logos/trademarks used in examples or screenshots without permission

**Code** copied in without checking the source license
- Snippets pasted from Stack Overflow, tutorials, or another repo without confirming the license is compatible with this repo's `LICENSE`
- A GPL/AGPL-licensed snippet or file dropped into first-party `src/`/`packs/` code, which would obligate the whole file/target under that license
- Vendored third-party code pasted inline instead of pinned as a proper `third_party/` submodule with its own `LICENSE` file kept alongside it (the existing pattern - see `third_party/*/LICENSE*`)

## Before staging or committing

- [ ] New file outside `third_party/` - does it match a RISK category above? Open it, don't just glance at the name.
- [ ] Any diff line that looks like a token/key/hash - stop, do not commit, move it to an env var or CI secret.
- [ ] Test fixture "realism" - is it synthesized, or copied from something real?
- [ ] Screenshot/log/crash-dump added for debugging this session - delete it once the fix lands, don't let it slip into the commit.
- [ ] New asset or pasted code snippet - do we actually have the right to redistribute it under this repo's license?

## Sources

[.gitignore](../../.gitignore), [LICENSE](../../LICENSE), [tools/hooks/pre-commit.sh](../../tools/hooks/pre-commit.sh) (style/EOF gate today - no secret or license scan yet), [rigkit-build](../rigkit-build/SKILL.md#git-commits) for the commit-message / local-CI gate.
