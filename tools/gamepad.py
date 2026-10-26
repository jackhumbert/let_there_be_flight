"""Drive a virtual Xbox 360 controller (ViGEmBus + vgamepad) for in-game tests.

  python tools/gamepad.py serve                  # keep one pad plugged in, run commands as they arrive
  python tools/gamepad.py "tap l3 120"           # send commands to the running server
  python tools/gamepad.py "hold l3 800; wait 500; stick l 0 1 1500"

Commands (`;`-separated, run in order):
  tap <button> [ms]          press and release (default 100 ms)
  hold <button> <ms>         same as tap, for long presses
  down <button> / up <button>
  stick <l|r> <x> <y> <ms>   deflect a stick (-1..1) for ms, then center it
  trigger <l|r> <0..1> <ms>  pull a trigger for ms, then release
  wait <ms>
Buttons: a b x y lb rb l3 r3 start back up down left right guide.

The server must be running before commands are sent; the game only notices a controller that
was plugged in while it runs, and unplugging between commands would look like a disconnect.
Needs the ViGEmBus driver (installed with `pip install vgamepad` on first import).
"""

import os
import sys
import time
from pathlib import Path

DIR = Path(__file__).resolve().parent.parent / "build" / "gamepad"
REQUEST = DIR / "request.txt"
RESPONSE = DIR / "response.txt"


def server() -> None:
    import vgamepad as vg

    B = vg.XUSB_BUTTON
    buttons = {
        "a": B.XUSB_GAMEPAD_A, "b": B.XUSB_GAMEPAD_B, "x": B.XUSB_GAMEPAD_X, "y": B.XUSB_GAMEPAD_Y,
        "lb": B.XUSB_GAMEPAD_LEFT_SHOULDER, "rb": B.XUSB_GAMEPAD_RIGHT_SHOULDER,
        "l3": B.XUSB_GAMEPAD_LEFT_THUMB, "r3": B.XUSB_GAMEPAD_RIGHT_THUMB,
        "start": B.XUSB_GAMEPAD_START, "back": B.XUSB_GAMEPAD_BACK, "guide": B.XUSB_GAMEPAD_GUIDE,
        "up": B.XUSB_GAMEPAD_DPAD_UP, "down": B.XUSB_GAMEPAD_DPAD_DOWN,
        "left": B.XUSB_GAMEPAD_DPAD_LEFT, "right": B.XUSB_GAMEPAD_DPAD_RIGHT,
    }
    pad = vg.VX360Gamepad()
    pad.reset()
    pad.update()
    DIR.mkdir(parents=True, exist_ok=True)
    last = REQUEST.read_text(encoding="utf-8").partition("\n")[0] if REQUEST.exists() else None
    print("virtual pad plugged in; waiting for commands", flush=True)

    def press(name: str, down: bool) -> None:
        (pad.press_button if down else pad.release_button)(button=buttons[name])
        pad.update()

    def run(line: str) -> None:
        for cmd in filter(None, (c.strip() for c in line.split(";"))):
            w = cmd.split()
            op = w[0]
            if op in ("tap", "hold"):
                press(w[1], True)
                time.sleep((float(w[2]) if len(w) > 2 else 100) / 1000)
                press(w[1], False)
            elif op in ("down", "up"):
                press(w[1], op == "down")
            elif op == "stick":
                x, y = float(w[2]), float(w[3])
                (pad.left_joystick_float if w[1] == "l" else pad.right_joystick_float)(x_value_float=x, y_value_float=y)
                pad.update()
                time.sleep(float(w[4]) / 1000)
                (pad.left_joystick_float if w[1] == "l" else pad.right_joystick_float)(x_value_float=0.0, y_value_float=0.0)
                pad.update()
            elif op == "trigger":
                (pad.left_trigger_float if w[1] == "l" else pad.right_trigger_float)(value_float=float(w[2]))
                pad.update()
                time.sleep(float(w[3]) / 1000)
                (pad.left_trigger_float if w[1] == "l" else pad.right_trigger_float)(value_float=0.0)
                pad.update()
            elif op == "wait":
                time.sleep(float(w[1]) / 1000)
            else:
                raise ValueError(f"unknown command {cmd!r}")

    while True:
        time.sleep(0.02)
        try:
            text = REQUEST.read_text(encoding="utf-8")
        except OSError:
            continue
        rid, _, body = text.partition("\n")
        if not rid or rid == last:
            continue
        last = rid
        try:
            run(body)
            status, msg = "ok", body.strip()
        except Exception as e:  # noqa: BLE001 - report every failure back to the caller
            status, msg = "error", str(e)
        RESPONSE.write_text(f"{rid}\t{status}\n{msg}", encoding="utf-8")


def send(line: str, timeout: float = 60.0) -> int:
    DIR.mkdir(parents=True, exist_ok=True)
    rid = str(time.time_ns())
    tmp = REQUEST.with_suffix(".tmp")
    tmp.write_text(f"{rid}\n{line}", encoding="utf-8")
    os.replace(tmp, REQUEST)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            head, _, body = RESPONSE.read_text(encoding="utf-8").partition("\n")
        except OSError:
            head = body = ""
        if head.startswith(rid + "\t"):
            status = head.split("\t", 1)[1]
            print(f"{status}: {body}")
            return 0 if status == "ok" else 1
        time.sleep(0.02)
    print("timeout: is `python tools/gamepad.py serve` running?", file=sys.stderr)
    return 2


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    if sys.argv[1] == "serve":
        server()
    else:
        sys.exit(send(" ".join(sys.argv[1:])))
