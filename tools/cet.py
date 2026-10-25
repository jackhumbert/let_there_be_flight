"""Run Lua inside the running game through the claude_bridge CET mod.

  python tools/cet.py install                 # copy tools/cet_bridge into CET's mods folder
  python tools/cet.py run "Game.GetPlayer()"  # an expression's value is returned
  python tools/cet.py run -f script.lua       # a whole chunk from a file (or `run -` for stdin)
  python tools/cet.py wait --timeout 180      # block until the bridge answers (game loaded)
  python tools/cet.py mcp                     # stdio MCP server exposing a `cet_lua` tool

The game must be running with CET loaded (the mod is picked up at CET init, or via
"Reload all mods" in the CET overlay). Requests run on the game's main thread, polled
every 0.1 s of game time, so nothing runs while the game is frozen on a loading screen
or a breakpoint.

Avoid GameDump()/Dump() on raw struct returns (e.g. a Matrix from a native getter):
that crashed CET 1.37.1 on 2026-09-30. Read the fields you need instead.
"""

import argparse
import os
import shutil
import sys
import time
from pathlib import Path

GAME_DIR = Path(os.environ.get("CYBERPUNK_2077_GAME_DIR", r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077"))
MOD_DIR = GAME_DIR / "bin" / "x64" / "plugins" / "cyber_engine_tweaks" / "mods" / "claude_bridge"
SOURCE_DIR = Path(__file__).resolve().parent / "cet_bridge"
REQUEST = MOD_DIR / "request.txt"
RESPONSE = MOD_DIR / "response.txt"


class BridgeError(Exception):
    pass


def install() -> None:
    MOD_DIR.mkdir(parents=True, exist_ok=True)
    for f in SOURCE_DIR.iterdir():
        shutil.copyfile(f, MOD_DIR / f.name)
    print(f"installed to {MOD_DIR}; restart the game or use 'Reload all mods' in the CET overlay")


def game_running() -> bool:
    if os.name != "nt":
        return True
    import subprocess
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Cyberpunk2077.exe", "/NH"], capture_output=True, text=True).stdout
    return "Cyberpunk2077.exe" in out


def run_lua(code: str, timeout: float = 10.0) -> tuple[bool, str]:
    """Returns (ok, output). Raises BridgeError when the bridge does not answer."""
    if not (MOD_DIR / "init.lua").exists():
        raise BridgeError(f"claude_bridge not installed in {MOD_DIR}; run: python tools/cet.py install")
    if not game_running():
        raise BridgeError("Cyberpunk2077.exe is not running")
    req_id = f"{time.time_ns()}"
    tmp = REQUEST.with_suffix(".tmp")
    tmp.write_text(f"{req_id}\n{code}", encoding="utf-8")
    # the mod may have request.txt open for its poll at this instant; Windows then refuses the replace
    for attempt in range(50):
        try:
            os.replace(tmp, REQUEST)
            break
        except PermissionError:
            if attempt == 49:
                raise
            time.sleep(0.02)

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            text = RESPONSE.read_text(encoding="utf-8")
        except (FileNotFoundError, PermissionError):
            text = ""
        head, _, body = text.partition("\n")
        rid, _, status = head.partition("\t")
        if rid == req_id:
            return status == "ok", body
        time.sleep(0.05)
    raise BridgeError(f"no response within {timeout:.0f} s (game paused on a loading screen, CET not loaded, or the mod not reloaded?)")


def wait_ready(timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            ok, _ = run_lua("1", 2.0)
            if ok:
                return True
        except BridgeError:
            time.sleep(1.0)
    return False


def serve_mcp() -> None:
    from mcp.server.fastmcp import FastMCP

    server = FastMCP("cet")

    @server.tool()
    def cet_lua(code: str, timeout: float = 10.0) -> str:
        """Run Lua in the running Cyberpunk 2077 via Cyber Engine Tweaks (game main thread).

        `code` is an expression (its value is returned) or a chunk (use print/return).
        CET globals are available: Game, GetSingleton, NewObject, Vector4, etc.
        Do not GameDump() raw struct returns; it can crash CET.
        """
        try:
            ok, out = run_lua(code, timeout)
        except BridgeError as e:
            return f"bridge error: {e}"
        return out if ok else f"lua error: {out}"

    server.run()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("install")
    run = sub.add_parser("run")
    run.add_argument("code", nargs="?", help="Lua source, or - to read stdin")
    run.add_argument("-f", "--file", help="read the Lua chunk from this file")
    run.add_argument("--timeout", type=float, default=10.0)
    wait = sub.add_parser("wait")
    wait.add_argument("--timeout", type=float, default=180.0)
    sub.add_parser("mcp")
    args = parser.parse_args()

    if args.cmd == "install":
        install()
    elif args.cmd == "run":
        if args.file:
            code = Path(args.file).read_text(encoding="utf-8")
        elif args.code == "-":
            code = sys.stdin.read()
        elif args.code:
            code = args.code
        else:
            parser.error("run needs Lua source, - or -f FILE")
        try:
            ok, out = run_lua(code, args.timeout)
        except BridgeError as e:
            sys.exit(f"bridge error: {e}")
        print(out)
        sys.exit(0 if ok else 1)
    elif args.cmd == "wait":
        if wait_ready(args.timeout):
            print("ready")
        else:
            sys.exit("bridge did not answer")
    else:
        serve_mcp()


if __name__ == "__main__":
    main()
