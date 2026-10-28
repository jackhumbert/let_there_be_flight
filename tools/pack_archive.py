"""Rebuild src/wolvenkit/packed/archive/pc/mod/let_there_be_flight.archive with the WolvenKit CLI.

Steps:
  1. Deserialize each localization json under src/wolvenkit/source/raw/user/jackhumbert/localization/
     (`*.json.json`) whose content differs from its src/wolvenkit/source/archive/ file.
  2. Pack src/wolvenkit/source/archive/ into let_there_be_flight.archive.
  3. Unbundle the new archive and check it holds exactly the files in source/archive/.

Only localization is imported. For everything else the CR2W files in source/archive/ are the
source of truth: the other raw files are stale Wolvenkit exports (flight_hud.inkwidget.json is
from 2022 and predates the packed widget), so edit those assets in the Wolvenkit GUI.

  python tools/pack_archive.py           # import + pack + verify
  python tools/pack_archive.py --check   # only report raw json that differs from source/archive
  python tools/pack_archive.py --out build/archive/let_there_be_flight.archive --oodle path/to/oo2ext_7_win64.dll

The CLI is --cli, then WOLVENKIT_CLI, then %LOCALAPPDATA%/Programs/WolvenKit.Console/<version>/.
If missing it is downloaded from the WolvenKit GitHub release (the NuGet `wolvenkit.cli`
dotnet tool stopped being a valid tool package after 8.x). Packing needs the game's
oo2ext_7_win64.dll next to the CLI (Oodle; not redistributable): --oodle copies it there. CMake
runs this at build time when it has a CLI or the dll (LTBF_WOLVENKIT_CLI / LTBF_OODLE_DLL); CI
gets the dll from a private repo (.github/workflows).
"""

import argparse
import filecmp
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

WOLVENKIT_VERSION = "9.0.1"
ROOT = Path(__file__).resolve().parent.parent
WK = ROOT / "src" / "wolvenkit"
RAW = WK / "source" / "raw" / "user"
LOCALIZATION = RAW / "jackhumbert" / "localization"
SOURCE = WK / "source" / "archive"
OUTPUT = WK / "packed" / "archive" / "pc" / "mod" / "let_there_be_flight.archive"


def find_cli(given: str | None = None) -> Path:
    env = given or os.environ.get("WOLVENKIT_CLI")
    if env:
        return Path(env)
    install = Path(os.environ["LOCALAPPDATA"]) / "Programs" / "WolvenKit.Console" / WOLVENKIT_VERSION
    exe = install / "WolvenKit.CLI.exe"
    if exe.exists():
        return exe
    url = (f"https://github.com/WolvenKit/WolvenKit/releases/download/{WOLVENKIT_VERSION}/"
           f"WolvenKit.Console-{WOLVENKIT_VERSION}.zip")
    print(f"downloading {url}")
    install.mkdir(parents=True, exist_ok=True)
    zip_path = install / "cli.zip"
    urllib.request.urlretrieve(url, zip_path)
    with zipfile.ZipFile(zip_path) as z:
        z.extractall(install)
    zip_path.unlink()
    return exe


def run(cli: Path, *args: str) -> None:
    # The CLI exits 0 with no output on some bad invocations, so callers verify results.
    result = subprocess.run([str(cli), *args, "-v", "Minimal"], capture_output=True, text=True)
    if result.returncode != 0 or "Error" in result.stdout:
        sys.exit(f"WolvenKit.CLI {' '.join(args)} failed ({result.returncode}):\n{result.stdout}{result.stderr}")


def raw_files() -> list[tuple[Path, Path]]:
    """(raw json, source/archive target) pairs."""
    pairs = []
    for raw in sorted(LOCALIZATION.glob("*.json.json")):
        target = SOURCE / "user" / raw.relative_to(RAW).with_suffix("")
        pairs.append((raw, target))
    return pairs


def serialize(cli: Path, cr2w: Path, out: Path) -> object:
    out.mkdir(parents=True)
    run(cli, "cr2w", "-s", str(cr2w), "-o", str(out))
    return json.loads(next(out.glob("*.json")).read_text(encoding="utf-8"))["Data"]["RootChunk"]


def import_raw(cli: Path, raw: Path, out: Path) -> Path:
    out.mkdir(parents=True)
    run(cli, "cr2w", "-d", str(raw), "-o", str(out))
    built = out / raw.with_suffix("").name
    if not built.exists():
        sys.exit(f"import produced nothing for {raw}")
    return built


def check(cli: Path, tmp: Path) -> list[tuple[Path, Path]]:
    """(built CR2W, target) for raw jsons whose content differs from source/archive.

    Compared after a full round trip, since serializing adds default fields the raw json omits.
    """
    stale = []
    for raw, target in raw_files():
        built = import_raw(cli, raw, tmp / "import" / raw.name)
        if not target.exists() or serialize(cli, built, tmp / "new" / raw.name) != serialize(cli, target, tmp / "old" / raw.name):
            stale.append((built, target))
    return stale


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="report stale raw json and exit 1 if any")
    parser.add_argument("--drop", action="append", default=[], metavar="DEPOT_PATH",
                        help="a file the archive loses on purpose (forward or back slashes; repeatable)")
    parser.add_argument("--out", type=Path, default=OUTPUT, help="where to write the archive (default: the packed path in src/wolvenkit)")
    parser.add_argument("--cli", help="WolvenKit.CLI.exe (default: WOLVENKIT_CLI, else the pinned version, downloaded if missing)")
    parser.add_argument("--oodle", type=Path, help="the game's oo2ext_7_win64.dll, copied next to the CLI if it is not there")
    args = parser.parse_args()

    cli = find_cli(args.cli)
    if args.oodle:
        target = cli.parent / args.oodle.name
        if not target.exists():
            shutil.copyfile(args.oodle, target)
    out = args.out.resolve()
    with tempfile.TemporaryDirectory() as tmp_str:
        tmp = Path(tmp_str)
        stale = check(cli, tmp)
        for _, target in stale:
            print(f"stale: {target.relative_to(ROOT)}")
        if args.check:
            sys.exit(1 if stale else 0)

        for built, target in stale:
            shutil.copyfile(built, target)
            print(f"imported: {target.relative_to(ROOT)}")

        # pack names the archive after the input folder
        pack_out = tmp / "pack"
        run(cli, "pack", str(SOURCE), "-o", str(pack_out))
        archive = pack_out / "archive.archive"

        unpacked = tmp / "verify"
        run(cli, "unbundle", str(archive), "-o", str(unpacked))
        want = sorted(p.relative_to(SOURCE) for p in SOURCE.rglob("*") if p.is_file())
        # never ship an archive that lost files: source/archive missed 7 of the 45 on one checkout
        previous = OUTPUT if OUTPUT.exists() else out
        if previous.exists():
            committed = tmp / "committed"
            run(cli, "unbundle", str(previous), "-o", str(committed))
            dropped = {p.relative_to(committed) for p in committed.rglob("*") if p.is_file()} - set(want)
            dropped = {p for p in dropped if p.as_posix().lower() not in {d.replace("\\", "/").lower() for d in args.drop}}
            if dropped:
                sys.exit(f"source/archive lacks files the committed archive has (copy them in first): {sorted(dropped)}")
        got = sorted(p.relative_to(unpacked) for p in unpacked.rglob("*") if p.is_file())
        if want != got:
            sys.exit(f"archive file list mismatch:\n  missing {set(want) - set(got)}\n  extra {set(got) - set(want)}")
        bad = [p for p in want if not filecmp.cmp(SOURCE / p, unpacked / p, shallow=False)]
        if bad:
            sys.exit(f"archive content mismatch: {bad}")

        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(archive, out)
        print(f"packed {len(want)} files -> {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
