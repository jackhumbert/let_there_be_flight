"""A recorded run's video with LTBF's own FMOD sound as its audio: the plugin's stems
(FlightAudio's AcousticsStems writes red4ext/logs/ltbf_stems_<time>_<dry|wet|bus>.wav: <base>_dry.wav, _wet.wav, _bus.wav and <base>.json) lined up with
a screen recording take, in one mp4 with an audio track each.

    python tools/acoustics/stems_video.py <out>/<take> <stems base> [--first wet] [--shift s]

Tracks, --first first (the one a player plays unless told otherwise): wet (the thrusters with the
echoes, then the bus), dry (the thrusters and the bus without), echoes (wet minus dry, at its own
gain: the echoes alone, quiet in the open), bus (LTBF's other FMOD sounds), game (the recording's own
audio). Wet and dry take one gain, so they compare as they are. --small re-encodes the video to fit
SendUserFile's 30 MB for about 90 s.

The stems start at the wall clock time in <base>.json; the recording's video at <take>.wall. That
is good to the mixer's latency, so the lag within 1 s where the thrusters' loudness follows the
recording's best refines it (printed; --shift s adds to it).
"""

import argparse
import json
import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/, for common
from common import paths  # noqa: E402

TRACKS = ("wet", "dry", "echoes", "bus", "game")


def read_float_wav(path):
    """The plugin's stems: 44-byte header, stereo 32-bit float."""
    with open(path, "rb") as f:
        head = f.read(44)
        rate = int.from_bytes(head[24:28], "little")
        x = np.frombuffer(f.read(), "<f4")
    return x[: len(x) // 2 * 2].reshape(-1, 2), rate


def read_pcm_wav(path):
    import wave
    with wave.open(path) as w:
        sr, ch = w.getframerate(), w.getnchannels()
        x = np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.float32) / 32768
    return x.reshape(-1, ch), sr


def loudness(x, sr, hop_s=0.01):
    hop = int(sr * hop_s)
    m = x.mean(1)
    k = len(m) // hop
    return 20 * np.log10(np.sqrt((m[: k * hop].reshape(k, hop) ** 2).mean(1)) + 1e-5)


def refine(wet, wet_sr, game, game_sr, coarse):
    """The lag (s) near coarse where the stem's loudness lines up best with the recording's, and the
    correlation there: the stem at video time coarse + lag."""
    a, b = loudness(wet, wet_sr), loudness(game, game_sr)
    best = (-2.0, 0.0)
    for lag in np.arange(-1.0, 1.0, 0.01):
        start = int(round((coarse + lag) / 0.01))
        # stem frame i is recording frame start + i
        lo, hi = max(0, -start), min(len(a), len(b) - start)
        if hi - lo < 500:
            continue
        x, y = a[lo:hi], b[lo + start : hi + start]
        r = np.corrcoef(x, y)[0, 1]
        if r > best[0]:
            best = (r, lag)
    return best


def place(label, at):
    """An ffmpeg filter that puts the audio labelled label at video time at (s)."""
    if at >= 0:
        return f"[{label}]adelay={at * 1000:.0f}:all=1"
    return f"[{label}]atrim=start={-at:.3f},asetpts=PTS-STARTPTS"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("take", help="the recording's <out>/<name> (.mp4, .wav, .wall, .offset beside it)")
    ap.add_argument("stems", help="the stems' base path (the path AcousticsStems returned, up to _dry.wav etc.)")
    ap.add_argument("--first", default="wet", help="the track played first: " + ", ".join(TRACKS) + " or an --extra's name")
    ap.add_argument("--shift", type=float, default=0.0, help="seconds added to the stems' place")
    ap.add_argument("--out", help="default <take>_fx.mp4")
    ap.add_argument("--small", action="store_true", help="video re-encoded at 1.7 Mbit/s")
    ap.add_argument("--extra", action="append", default=[],
                    help="name=path: another float WAV on the stems' timeline (engine_synth's render of the run), "
                         "at the dry and wet tracks' gain")
    args = ap.parse_args()

    with open(args.take + ".wall") as f:
        wall = float(f.read())
    with open(args.take + ".offset") as f:
        game_at = float(f.read())
    with open(args.stems + ".json") as f:
        info = json.load(f)
    coarse = (info["startWallOfDay"] - wall + 43200) % 86400 - 43200

    stems = {k: read_float_wav(f"{args.stems}_{k}.wav") for k in ("dry", "wet", "bus")}
    sr = stems["wet"][1]
    game, game_sr = read_pcm_wav(args.take + ".wav")
    # the recording's audio starts game_at s into the video
    r, lag = refine(stems["wet"][0] + stems["bus"][0], sr, game, game_sr, coarse - game_at)
    at = coarse + lag + args.shift
    print(f"stems {len(stems['wet'][0]) / sr:.1f} s, at {at:+.3f} s in the video (clocks {coarse:+.3f}, "
          f"loudness lag {lag:+.2f}, r {r:.2f})")
    if r < 0.5:
        print("  weak match: the clocks' place may be the better one (--shift to correct)")

    # wet and dry at one gain: the louder one's peak at -1 dBFS
    mixes = {"wet": stems["wet"][0] + stems["bus"][0], "dry": stems["dry"][0] + stems["bus"][0],
             "bus": stems["bus"][0]}
    peak = max(np.abs(mixes["wet"]).max(), np.abs(mixes["dry"]).max(), 1e-6)
    gain = 10 ** (-1 / 20) / peak
    bus_gain = 10 ** (-1 / 20) / max(np.abs(mixes["bus"]).max(), 1e-6)
    echoes_gain = 10 ** (-1 / 20) / max(np.abs(stems["wet"][0] - stems["dry"][0]).max(), 1e-6)
    print(f"  peak {20 * np.log10(peak):+.1f} dBFS, gain {20 * np.log10(gain):+.1f} dB")

    inputs = ["-i", args.take + ".mp4", "-i", f"{args.stems}_wet.wav", "-i", f"{args.stems}_dry.wav",
              "-i", f"{args.stems}_bus.wav", "-i", args.take + ".wav"]
    filters = [
        f"[1:a][3:a]amix=inputs=2:normalize=0,volume={gain:.4f}[w0]",
        f"[2:a][3:a]amix=inputs=2:normalize=0,volume={gain:.4f}[d0]",
        f"[3:a]volume={bus_gain:.4f}[b0]",
        f"[2:a]volume=-1[neg]",
        f"[1:a][neg]amix=inputs=2:normalize=0,volume={echoes_gain:.4f}[e0]",
    ]
    for name, src in (("wet", "w0"), ("dry", "d0"), ("echoes", "e0"), ("bus", "b0")):
        filters.append(f"{place(src, at)},apad[{name}]")
    filters.append(f"{place('4:a', game_at)},apad[game]")
    titles = {"wet": "LTBF with echoes", "dry": "LTBF dry", "echoes": "echoes only", "bus": "LTBF bus",
              "game": "game recording"}
    tracks = list(TRACKS)
    for k, extra in enumerate(args.extra):
        name, path = extra.split("=", 1)
        inputs += ["-i", path]
        filters.append(f"[{5 + k}:a]volume={gain:.4f},{place('x', at)[3:]},apad[{name}]")
        titles[name] = name
        tracks.append(name)
    if args.first not in tracks:
        sys.exit(f"--first {args.first}: no such track")
    order = [args.first] + [t for t in tracks if t != args.first]
    out = args.out or args.take + "_fx.mp4"
    cmd = [paths.FFMPEG, "-hide_banner", "-loglevel", "error", "-y", *inputs, "-filter_complex", ";".join(filters),
           "-map", "0:v"]
    for t in order:
        cmd += ["-map", f"[{t}]"]
    video = ["-c:v", "h264_nvenc", "-b:v", "1700k", "-maxrate", "2000k", "-bufsize", "4000k"] if args.small else ["-c:v", "copy"]
    cmd += [*video, "-c:a", "aac", "-b:a", "128k"]
    for i, t in enumerate(order):
        cmd += [f"-metadata:s:a:{i}", f"title={titles[t]}", f"-disposition:a:{i}", "default" if i == 0 else "0"]
    cmd += ["-shortest", out]
    subprocess.run(cmd, check=True)
    print("video:", out, "tracks", ", ".join(order))


if __name__ == "__main__":
    main()
