# Releasing

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, git-cliff changelog from conventional commits + `requirements.md`, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Checkout uses `fetch-depth: 0`; with a shallow clone git-cliff sees no history and the release body has no Changes section.
- Commit messages feed git-cliff (`cliff.toml`): use `feat:` / `fix:` prefixes for anything that should appear in the changelog. `chore:` and `ci:` are skipped.
- `git-cliff --current` (0.10.0) groups commits by date, not by tag ancestry: a release that merges older-dated commits gets them under the previous version, with the whole history. Likely fix, untested: generate from the tag range, e.g. `git-cliff --strip header --tag <tag> <prev-tag>..<tag>` with `<prev-tag>` from `git describe --tags --abbrev=0 <tag>^`. Check before the next release that merges older history. Whether a release is affected: `git rev-list --merges --count <prev-tag>..HEAD` is 0 and no commit in that range is dated before the previous tag's commit (true for the release after v0.3.20, so `--current` stays for it).
- The archive is packed in CI with a downloaded WolvenKit CLI and no secret (`CMAKE_CI_BUILD` turns `LTBF_PACK_ARCHIVE` on). The Build log says "Packing let_there_be_flight.archive", and both workflows fail when no archive was packed. Tag a prerelease first (`vX.Y.Z-rc.1`: no Nexus upload) to prove the pipeline.
- Nexus Mods: the same zip is uploaded by the release workflow; see "Nexus Mods" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208).

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Implemented in `release.yaml`: after the GitHub release step, `Nexus-Mods/upload-action` uploads `let_there_be_flight_vX.Y.Z.zip` as a new version of file id 2617843 (the main LTBF file; public id, hardcoded) with the git-cliff changelog, and bumps the mod version. The changelog call needs the v3 global mod id, not the site's game-scoped 5208, so a step resolves it via `GET /v3/games/cyberpunk2077/mods/5208` first. The action creates the file version before posting the changelog, so a changelog failure still leaves the file uploaded. It runs only for non-prerelease tags and only when the repo secret `NEXUSMODS_API_KEY` is set (https://www.nexusmods.com/settings/api-keys). Creating a new mod page is still manual.
