# Releasing

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, release notes from `docs/releases` or the commits, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Release notes are written by hand, one file per version: `docs/releases/vX.Y.Z.md`, for players, with sections such as `## New`, `## Fixes`, `## Upgrading from X.Y.Z` and `## Known issues`. A prerelease tag uses its version's file. Write it before the tag.
- Start each item of a change section with a bold headline that says what changed, as a sentence that stands on its own: `- **Fixed the black screen in the Arasaka Tower 3D arcade game.** <the details>`, not `**Arasaka Tower 3D black screen.**`. The headlines alone become the Nexus changelog and the file's description.
- `deps/cyberpunk_cmake/scripts/release_notes.py` turns the notes into three texts (run it locally with `--tag vX.Y.Z --repo-url <url>` to see them):
  - the GitHub release body: the notes as written, a link to every commit since the last full release, then `requirements.md`;
  - the Nexus changelog: one plain line per headline of the change sections (no Upgrading, Known issues or requirements). Nexus shows each line as an entry and renders no markdown;
  - the Nexus file description (the box under the file): the same lines as `* line`, cut to fit 255 characters. Nexus does not let it be changed after the upload.
- Without a notes file, all three come from the `feat:`, `fix:` and `perf:` commit subjects since the last full release (`git describe --exclude "*-*"`; a prerelease tag in between is not a version of its own), so write those subjects for players. `docs:`, `refactor:`, `build:`, `test:`, `chore:` and `ci:` are left out. The checkout uses `fetch-depth: 0` for that history.
- The archive is packed in CI with a downloaded WolvenKit CLI and no secret (`CMAKE_CI_BUILD` turns `LTBF_PACK_ARCHIVE` on). The Build log says "Packing let_there_be_flight.archive", and both workflows fail when no archive was packed. Tag a prerelease first (`vX.Y.Z-rc.1`: no Nexus upload) to prove the pipeline.
- Nexus Mods: the same zip is uploaded by the release workflow; see "Nexus Mods" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208).

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `display_name`, `description`, `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Implemented in `release.yaml`: after the GitHub release step, `Nexus-Mods/upload-action` uploads `let_there_be_flight_vX.Y.Z.zip` as a new version of file id 2617843 (the main LTBF file; public id, hardcoded) with the changelog lines and file description from the release notes, and bumps the mod version. The changelog call needs the v3 global mod id, not the site's game-scoped 5208, so a step resolves it via `GET /v3/games/cyberpunk2077/mods/5208` first. The action creates the file version before posting the changelog, so a changelog failure still leaves the file uploaded. It runs only for non-prerelease tags and only when the repo secret `NEXUSMODS_API_KEY` is set (https://www.nexusmods.com/settings/api-keys). Creating a new mod page is still manual.
