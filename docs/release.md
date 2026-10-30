# Releasing

## Cutting a release

Releases are fully automated by `.github/workflows/release.yaml` on a `v*.*.*` tag push (windows-latest, Ninja, RelWithDebInfo, git-cliff changelog from conventional commits + `requirements.md`, uploads `let_there_be_flight_vX.Y.Z.zip` and `_pdb.zip`, release name `vX.Y.Z for <game version>`). Tags with `-` or `_` are marked prerelease.

```
git tag vX.Y.Z && git push origin main vX.Y.Z
```

- Before tagging: submodule pins committed and pushed to their own repos (CI clones with `submodules: true` over HTTPS; every `.gitmodules` URL must be publicly clonable). `build.yaml` on the push must be green.
- Checkout uses `fetch-depth: 0`; with a shallow clone git-cliff sees no history and the release body has no Changes section (every release through v0.3.18 was hand-edited for that reason).
- Commit messages feed git-cliff (`cliff.toml`): use `feat:` / `fix:` prefixes for anything that should appear in the changelog. `chore:` and `ci:` are skipped.
- `git-cliff --current` (0.10.0) groups commits by date, not by tag ancestry: v0.3.20 merged 41 older-dated commits from jordan, and the generated notes put them under [0.3.19] and included the whole history. The GitHub notes were rewritten by hand from `git log v0.3.19..v0.3.20`; the Nexus changelog (append-only via API) still has the long text. Likely fix, untested: generate from the tag range, e.g. `git-cliff --strip header --tag <tag> <prev-tag>..<tag>` with `<prev-tag>` from `git describe --tags --abbrev=0 <tag>^`. Check before the next release that merges older history.
- Release-note history: v0.3.17 (2025-08-29) "for 2.30+" - disabled thruster customizations (crystal coat crash), redscript 1.0 update, Limited HUD incompatibility.
- Nexus Mods: manual upload of the same zip today. See "Nexus" below.

## Nexus Mods

Mod page: https://www.nexusmods.com/cyberpunk2077/mods/5208 (mod ID 5208). Uploads have been manual so far.

Nexus now has an official Upload API (REST v3, open beta since 2026-03; personal API key in an `apikey` header, no cookies). Two supported ways to automate:

- GitHub Action `Nexus-Mods/upload-action` (pin a tag, currently `v1.0.0-beta.10`). Inputs: `api_key`, `file_id` (the v3 mod-file chain id from the Files tab "Advanced/API Info"), `filename`, `version`, optional `mod_id` + `changelog`, `update_mod_version`, `archive_existing_version`. Reference workflow: https://github.com/Nexus-Mods/API-Example.
- CLI for local use: `dotnet tool install -g BUTR.NexusUploader` then `unex upload 5208 <zip> -v <ver> --file-id <id>` (v4 uses the official API).

Implemented in `release.yaml`: after the GitHub release step, `Nexus-Mods/upload-action` uploads `let_there_be_flight_vX.Y.Z.zip` as a new version of file id 2617843 (the main LTBF file; public id, hardcoded) with the git-cliff changelog, and bumps the mod version. The changelog call needs the v3 global mod id, not the site's game-scoped 5208, so a step resolves it via `GET /v3/games/cyberpunk2077/mods/5208` first. The action creates the file version before posting the changelog, so a changelog failure still leaves the file uploaded (happened on v0.3.18). It runs only for non-prerelease tags and only when the repo secret `NEXUSMODS_API_KEY` is set (https://www.nexusmods.com/settings/api-keys). Creating a new mod page is still manual.

Manual API use (key in the `NEXUSMODS_API_KEY` user environment variable; read it with `[Environment]::GetEnvironmentVariable('NEXUSMODS_API_KEY','User')`, send as an `apikey` header, base `https://api.nexusmods.com/v3`; schema in `openapi.yaml` at the repo root, untracked):
- v3 mod id for 5208 is `14315126002776` (`GET /games/cyberpunk2077/mods/5208`).
- `GET /mod-files/2617843/versions` lists uploaded versions; `POST /mods/14315126002776/changelogs` with `{version, changelog}` appends notes (additive, cannot edit); `PUT /mod-files/2617843` with `{name}` renames the persistent file entry.
- 2026-09-13: the 0.3.18 changelog was posted this way after the action's changelog step failed on the wrong mod id.

Reading Nexus comments: the Posts tab and forums are Cloudflare-gated (plain HTTP, reader proxies, and the v2 GraphQL `searchComments` all fail), so `python tools/maintenance/nexus_comments.py --mod 5208 --since 2025-07-01` drives a real Chrome over the DevTools protocol (needs `pip install websocket-client`): it opens the Posts tab, reads the thread id, and pages the comment endpoint with in-page XHR into a JSON file (author, date, text, replies). Thread ids: LTBF 12009388, Mod Settings 11748838, In-World Navigation 11517568, Input Loader 11989233. Roughly 10 top-level comments per page; LTBF had 4,833 comments total on 2026-09-13.
