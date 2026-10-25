"""Run Lua inside the running game through the claude_bridge CET mod and print the result.

The mod (bin/x64/plugins/cyber_engine_tweaks/mods/claude_bridge/init.lua) polls request.txt every
0.1 s on the game's main thread: first line an id, the rest Lua source. It answers in response.txt:
first line "<id>\t<ok|error>", then captured print() output and the chunk's return values. A bare
expression is returned ("Game.GetPlayer()"), anything else runs as a chunk ("local x = 1; return x").

usage:
  python tools/cet.py "<lua>"            run inline Lua
  python tools/cet.py -f script.lua      run a file
  python tools/cet.py --wait-ready       wait until the bridge answers (game reached the main menu or later)
Exit code 0 on ok, 1 on a Lua error, 2 on timeout (game not running, or CET not loaded yet).
"""
import argparse
import os
import sys
import time

GAME_DIR = os.environ.get("CYBERPUNK_2077_GAME_DIR", r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077")
BRIDGE = os.path.join(GAME_DIR, "bin", "x64", "plugins", "cyber_engine_tweaks", "mods", "claude_bridge")


def run(code, timeout):
    rid = str(time.time_ns())
    tmp = os.path.join(BRIDGE, "request.txt.tmp")
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        f.write(rid + "\n" + code)
    # the mod may have request.txt open for its poll at this instant; Windows then refuses the replace
    for attempt in range(50):
        try:
            os.replace(tmp, os.path.join(BRIDGE, "request.txt"))
            break
        except PermissionError:
            if attempt == 49:
                raise
            time.sleep(0.02)
    deadline = time.monotonic() + timeout
    resp = os.path.join(BRIDGE, "response.txt")
    while time.monotonic() < deadline:
        try:
            with open(resp, encoding="utf-8", errors="replace") as f:
                text = f.read()
        except OSError:
            text = ""
        head, _, body = text.partition("\n")
        if head.startswith(rid + "\t"):
            return head.split("\t", 1)[1], body
        time.sleep(0.05)
    return None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("code", nargs="?")
    ap.add_argument("-f", "--file")
    ap.add_argument("-t", "--timeout", type=float, default=10.0)
    ap.add_argument("--wait-ready", action="store_true", help="poll until the bridge answers, up to --timeout")
    args = ap.parse_args()

    if args.wait_ready:
        deadline = time.monotonic() + args.timeout
        while time.monotonic() < deadline:
            status, _ = run("1", 2.0)
            if status == "ok":
                print("ready")
                return 0
        print("bridge did not answer", file=sys.stderr)
        return 2

    code = open(args.file, encoding="utf-8").read() if args.file else args.code
    if not code:
        ap.error("no Lua given")
    status, body = run(code, args.timeout)
    if status is None:
        print("timeout: no answer from claude_bridge (game not running, or still loading)", file=sys.stderr)
        return 2
    print(body)
    return 0 if status == "ok" else 1


if __name__ == "__main__":
    sys.exit(main())
