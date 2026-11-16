# Installed-title metadata discovery

**SPDW Factory · Read only, bounded, and boring in all the right places.**

## Supported first source

The first discovery adapter will inspect the conventional installed-title layout:

`ux0:/app/<TITLE_ID>/sce_sys/param.sfo`

The PS Vita filesystem reference documents `ux0:app/<titleid>/` as the application directory. A VPK's standard anatomy includes `sce_sys/param.sfo`, which stores title ID, app version and app name metadata:

- [PS Vita filesystem layout](https://www.psdevwiki.com/vita/index.php/Files_on_the_PS_Vita)
- [VPK / PARAM.SFO anatomy](https://github.com/robin994/PSVita-Dev-Wiki/blob/main/02-vitasdk/03-homebrew-app-anatomy.md)

The portable parser in this repository consumes bytes only. It does not open directories, construct paths, follow links, mount partitions or call privileged services. A later Vita adapter can use the documented user filesystem APIs `sceIoDopen`, `sceIoDread` and `sceIoDclose` to enumerate a chosen root, and ordinary file I/O to read a bounded `param.sfo`:

- [VitaSDK directory-entry API](https://docs.vitasdk.org/group__SceDirEntUser.html)
- [VitaSDK file I/O API](https://docs.vitasdk.org/group__SceFcntlUser.html)

## PARAM.SFO fields

The parser currently requires:

| Field | Catalogue mapping | Rule |
|---|---|---|
| `TITLE_ID` | `GameRecord::titleId` | Exactly nine alphanumeric characters |
| `TITLE` | `GameRecord::title` | Required, bounded UTF-8 string |
| `CATEGORY` | `GameRecord::category` | Optional, bounded string |
| `APP_VER` | `GameRecord::version` | Optional, bounded string |

The parser checks the PSF magic/version, entry table bounds, key table terminators, data offsets and lengths, string format, duplicate recognized keys and output capacities. Failed parses leave the caller's existing record untouched. Mutable user state such as favourites and last-played timestamps is never imported from package metadata.

## Deliberate boundaries

- The first filesystem adapter will be read-only and restricted to an explicitly selected application root.
- It will validate each directory name as a title ID before constructing a child path.
- It will read only regular files within a fixed maximum size; no symlink following or recursive traversal.
- It will not inspect private Shell databases such as `ur0:shell/db/app.db` as a primary API. Reverse-engineered database internals are not a stable public homebrew contract.
- `sceAppMgrAppParamGetString` is documented for retrieving SFO parameters for a process ID; it is not an installed-title enumeration API. See [VitaSDK SceAppMgr user API](https://docs.vitasdk.org/group__SceAppMgrUser.html).
- No title launch API is used by this parser. Launch is a separate adapter and release gate.

## Validation status

The portable parser and hostile fixture tests are implemented in the feature branch. The Vita filesystem enumerator, physical Vita fixture corpus, artwork discovery and launch/return lifecycle remain outstanding. Host tests do not certify device behavior.
