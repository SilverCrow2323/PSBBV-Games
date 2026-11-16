# PSBBV Games — Roadmap

**SPDW Factory · Catalogue first, launch safely.**

## Phase 0 — Contract and host foundation

- [x] Define Games node ownership and Core integration boundary.
- [x] Add provisional PSBBV node manifest and schema validation CI.
- [x] Add portable fixed-capacity catalogue model.
- [x] Validate Vita title IDs and bounded metadata fields.
- [x] Implement unique upsert, removal, lookup and case-insensitive search.
- [x] Implement favourites, recently-played timestamps and deterministic sorting.
- [x] Add host tests and GitHub Actions build/test workflow.

## Phase 1 — Installed-title discovery

- [x] Document the conventional ux0:/app/<TITLE_ID>/sce_sys/param.sfo source and relevant VitaSDK interfaces.
- [x] Add host fixtures for valid, partial, malformed and adversarial PARAM.SFO metadata.
- [x] Implement a bounded, allocation-free parser for the supported PARAM.SFO string fields.
- [ ] Map parsed records into the catalogue without following arbitrary paths or symlinks.
- [ ] Add rescan, stale-record and duplicate-source handling.

## Phase 2 — Library experience

- [ ] View models for all games, favourites, recently played and categories.
- [ ] Sorting and filtering controls exposed to the Core host UI.
- [ ] Artwork metadata and bounded cache policy.
- [ ] Empty, scanning, unavailable and error states.

## Phase 3 — Launch and return lifecycle

- [ ] Research and document supported Vita title-launch interfaces.
- [ ] Implement a narrow Vita launch adapter with explicit result codes.
- [ ] Preserve enough host state to recover after a title returns.
- [ ] Test missing title, malformed ID, launch rejection and unexpected return.
- [ ] Physical Vita tests for repeated launch/return and low-storage conditions.

## Quality rule

Catalogue host tests prove data-model behavior only. They do not prove that installed titles are discovered or launch correctly. No launch capability is marked ready before physical Vita validation.
