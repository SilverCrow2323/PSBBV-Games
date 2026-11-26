# PSBBV Games

**SPDW Factory · Your library, ready to play.**

PSBBV Games is the game-library and launch node in the PSBBV ecosystem. It discovers, organises and launches installed Vita titles while PSBBV Core remains responsible for global navigation, focus, lifecycle, permissions and node registration.

## Scope

- Discover installed titles from supported Vita metadata sources.
- Build a searchable, filterable catalogue with title, title ID, category, version and artwork metadata.
- Provide favourites, recently played, collections and sorting.
- Launch titles through documented Vita system interfaces.
- Track launch failures and return-to-PSBBV behavior without destabilising the shell.
- Expose a stable catalogue API to PSBBV Core without owning global navigation.

## Architecture rules

- Start as a built-in PSBBV node; no external dynamic loader or Python runtime on Vita.
- Follow the versioned Node API and manifest contract in [PSBBV Core](https://github.com/SilverCrow2323/PSBBV).
- Keep discovery, metadata parsing, catalogue state and launch adapter independently testable.
- Treat title IDs and system metadata as untrusted input; validate paths and never follow arbitrary symlinks.
- Do not download, redistribute or package commercial game content, keys or proprietary assets.
- Keep launch behavior explicit and recoverable; never silently terminate the shell after a failed launch.

## Delivery order

1. Document supported installed-title metadata sources and VitaSDK constraints.
2. Implement a portable metadata parser with fixture-based tests.
3. Add catalogue indexing, search, sorting and favourites.
4. Add a guarded Vita launch adapter and return-to-shell lifecycle.
5. Add artwork caching and recently played state.
6. Validate on physical Vita hardware across cold start, repeated launches and low-storage conditions.

## Quality gates

Host parser and catalogue tests, malformed-metadata fixtures, path-safety tests, VitaSDK build, VPK inspection and physical Vita launch/return tests. Emulator or CI success must not be reported as hardware certification.

## Status

The portable catalogue, bounded PARAM.SFO parser and provider-driven discovery coordinator are implemented with host fixtures. Discovery validates title IDs, rejects symlinked directories/files, bounds reads, preserves favourites, prunes stale records only after a complete scan and leaves the catalogue untouched after enumeration failure. The provider boundary rejects oversized directory scans after 512 entries and commits catalogue changes only after a complete scan. The VitaSDK filesystem adapter, artwork, title launch and return-to-shell lifecycle remain planned pending device integration and physical validation. English UI/docs and SPDW Factory identity are required.
