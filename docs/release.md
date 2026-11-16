# Releasing

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, git-cliff changelog from conventional commits + `requirements.md`, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Checkout uses `fetch-depth: 0`; with a shallow clone git-cliff sees no history and the release body has no Changes section.
- Commit messages feed git-cliff (`cliff.toml`): `feat:`, `fix:` and `perf:` appear in the changelog, written for players; `docs:`, `refactor:`, `build:`, `test:`, `chore:` and `ci:` are skipped.
- The changelog is everything since the last full release: the workflow finds that tag (`git describe --exclude "*-*"`) and runs `git-cliff --tag <tag> <previous>..<tag>`. A prerelease tag in between is not a version of its own (`ignore_tags`), so `vX.Y.Z-rc.1` and the `vX.Y.Z` after it both list the whole release. `git-cliff --current` (0.10.0) is not used: in CI it put every older version under the new one.
- Release notes are written by hand, one file per version: `docs/releases/vX.Y.Z.md` (what is new, fixes, upgrading, known issues, for players). The workflow puts it at the top of the GitHub release, folds the generated commit list under it, and sends the notes alone to Nexus. A prerelease tag uses its version's file. Without the file the commit list is the body: write it before the tag.
- The archive is packed in CI with a downloaded WolvenKit CLI and no secret (`CMAKE_CI_BUILD` turns `LTBF_PACK_ARCHIVE` on). The Build log says "Packing let_there_be_flight.archive", and both workflows fail when no archive was packed. Tag a prerelease first (`vX.Y.Z-rc.1`: no Nexus upload) to prove the pipeline.
- Nexus Mods: the same zip is uploaded by the release workflow; see "Nexus Mods" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208).

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Implemented in `release.yaml`: after the GitHub release step, `Nexus-Mods/upload-action` uploads `let_there_be_flight_vX.Y.Z.zip` as a new version of file id 2617843 (the main LTBF file; public id, hardcoded) with the git-cliff changelog, and bumps the mod version. The changelog call needs the v3 global mod id, not the site's game-scoped 5208, so a step resolves it via `GET /v3/games/cyberpunk2077/mods/5208` first. The action creates the file version before posting the changelog, so a changelog failure still leaves the file uploaded. It runs only for non-prerelease tags and only when the repo secret `NEXUSMODS_API_KEY` is set (https://www.nexusmods.com/settings/api-keys). Creating a new mod page is still manual.
