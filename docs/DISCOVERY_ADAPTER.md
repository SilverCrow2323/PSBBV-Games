# Installed-title discovery adapter contract

**SPDW Factory · Scan the library, not the entire console.**

## Current boundary

The portable coordinator receives an injected `DiscoverySource`; it does not call Vita filesystem APIs itself. This keeps the coordinator host-testable and makes the device adapter responsible for the exact mount and path policy.

The first intended device root is `ux0:/app`, with one candidate per direct child:

`ux0:/app/<TITLE_ID>/sce_sys/param.sfo`

See [VitaSDK directory iteration](https://docs.vitasdk.org/group__SceDirEntUser.html), [VitaSDK file I/O](https://docs.vitasdk.org/group__SceFcntlUser.html) and [installed application layout](https://www.psdevwiki.com/vita/index.php/Files_on_the_PS_Vita).

## Provider contract

- `open` establishes access to the one configured root.
- `next` returns one direct child name and its directory/symlink attributes; it must not recurse.
- The coordinator accepts only exactly nine alphanumeric title-ID names.
- `readParamSfo` receives only a validated title ID, and must resolve it beneath the configured root. It reports whether the metadata path is a regular file and whether it is a symlink.
- `close` is called after every successful open, including an interrupted enumeration.

## Hard limits and behavior

| Guard | Limit / behavior |
|---|---|
| PARAM.SFO scratch | Caller-owned, at most 16 KiB |
| Directory entries | 512 per scan |
| Unique valid title IDs | 128 per scan |
| Traversal | Direct children only; no recursion |
| Symlinks | Reject directory and metadata symlinks |
| File type | Accept regular metadata files only |
| Parse failure | Keep prior record if its valid title directory still exists |
| Complete scan | Commit staged catalogue and prune missing title IDs |
| Enumeration failure | Discard staged changes; preserve the live catalogue |

Existing favourites survive metadata refreshes because catalogue upsert retains the user's mutable state. Metadata never sets favourites or last-played timestamps.

Per-title read and parse failures are counted and skipped; they do not abort a complete directory enumeration. A provider-level enumeration failure, an exceeded global bound, or catalogue capacity rejection aborts the transaction.

## Not yet implemented

There is no concrete VitaSDK filesystem provider in this change. The next device-side step must use `sceIoDopen` / `sceIoDread` / `sceIoDclose`, inspect file type and symlink attributes, and read a size-bounded `param.sfo` with user-mode I/O. No physical Vita scan has been performed. Launching games remains a separate interface and lifecycle project.
