"""Publish a nav map bake for builds: upload it as a release asset of jackhumbert/cp2077_navmap and point
CMakeLists.txt at it (LTBF_NAVMAP_URL and LTBF_NAVMAP_SHA256), so the file and the pinned hash
cannot drift apart. Needs the GitHub CLI (`gh`) logged in with write access to that repo.

  python tools/navmap_bake/publish.py <navmap.bin> [--game 2.50] [--dry-run]

The release tag is <game>-<first 12 hex of the SHA-256>, so a new bake never replaces one an
older commit points at; publishing the same file again reuses its release. Commit CMakeLists.txt
afterwards.
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile

DATA_REPO = "jackhumbert/cp2077_navmap"
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CMAKE = os.path.join(REPO, "CMakeLists.txt")
URL_RE = re.compile(r'(set\(LTBF_NAVMAP_URL ")[^"]*(")')
SHA_RE = re.compile(r'(set\(LTBF_NAVMAP_SHA256 ")[0-9a-f]*(")')


def gh(*args, check=True):
    r = subprocess.run(["gh", *args], capture_output=True, text=True)
    if check and r.returncode:
        sys.exit(f"gh {' '.join(args[:2])} failed: {r.stderr.strip()}")
    return r


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("bake", help="the navmap.bin to publish (navmap.py's output)")
    ap.add_argument("--game", help="game version the bake is for (default: the one in the current URL)")
    ap.add_argument("--dry-run", action="store_true", help="print what would happen, upload and change nothing")
    args = ap.parse_args()

    data = open(args.bake, "rb").read()
    sha = hashlib.sha256(data).hexdigest()
    cmake = open(CMAKE, encoding="utf-8").read()
    old_url = URL_RE.search(cmake)
    if not old_url or not SHA_RE.search(cmake):
        sys.exit("LTBF_NAVMAP_URL / LTBF_NAVMAP_SHA256 not found in CMakeLists.txt")
    game = args.game or (re.search(r"/download/(?:navmap-)?(\d+(?:\.\d+)*)", old_url.group(0)) or [None, None])[1]
    if not game:
        sys.exit("give --game: the current URL names no game version")
    tag = f"{game}-{sha[:12]}"
    url = f"https://github.com/{DATA_REPO}/releases/download/{tag}/navmap.bin"
    print(f"{args.bake}: {len(data) / 2**20:.1f} MB, sha256 {sha}")
    print(f"release {DATA_REPO} {tag}")
    if args.dry_run:
        print(f"would set LTBF_NAVMAP_URL {url}")
        return

    if gh("release", "view", tag, "-R", DATA_REPO, check=False).returncode:
        notes = (f"Nav map bake for game {game}. let_there_be_flight's build embeds it "
                 f"(CMakeLists.txt, LTBF_NAVMAP_URL).\n\nSHA-256: {sha}\nSize: {len(data)} bytes")
        gh("release", "create", tag, "-R", DATA_REPO, "--title", tag, "--notes", notes)
    assets = gh("release", "view", tag, "-R", DATA_REPO, "--json", "assets", "-q", ".assets[].name").stdout.split()
    if "navmap.bin" not in assets:
        with tempfile.TemporaryDirectory() as tmp:
            upload = os.path.join(tmp, "navmap.bin")
            with open(upload, "wb") as f:
                f.write(data)
            gh("release", "upload", tag, upload, "-R", DATA_REPO)
        print("uploaded navmap.bin")
    else:
        print("navmap.bin already on the release")

    # check what is served before pointing the build at it
    with tempfile.TemporaryDirectory() as tmp:
        gh("release", "download", tag, "-R", DATA_REPO, "-p", "navmap.bin", "-D", tmp)
        got = hashlib.sha256(open(os.path.join(tmp, "navmap.bin"), "rb").read()).hexdigest()
    if got != sha:
        sys.exit(f"the release serves sha256 {got}, not {sha}: CMakeLists.txt left as it was")

    cmake = URL_RE.sub(lambda m: m.group(1) + url + m.group(2), cmake)
    cmake = SHA_RE.sub(lambda m: m.group(1) + sha + m.group(2), cmake)
    with open(CMAKE, "w", encoding="utf-8", newline="") as f:
        f.write(cmake)
    print(f"CMakeLists.txt: LTBF_NAVMAP_URL {url}")
    print("commit CMakeLists.txt; builds without LTBF_NAVMAP_FILE now download this bake")


if __name__ == "__main__":
    main()
