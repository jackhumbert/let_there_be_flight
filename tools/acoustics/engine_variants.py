"""One recorded run heard with the engine synth's switches each way: the FMOD event's own effects kept or
dropped (EngineSynth::Character: cone, distortion, darkness, grit) and air absorption on the echoes,
each rendered from the run's own parameter log through the echoes along its own path
(engine_synth --acoustics), at the FMOD event's loudness, as audio tracks of the run's video.

    python tools/acoustics/engine_variants.py <run dir> [--take free_roam] [--navmap navmap.bin] [--tour 10]

<run dir> is a test driver's --out (stems.path and <take>.mp4 / .wav / .wall / .offset in it). The first
track is a tour: each variant in turn for --tour seconds, in the order printed; then each variant
whole, the FMOD event as it played (its wet stem), and the game recording.
"""

import argparse
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)
from common import paths  # noqa: E402
import stems_video as sv  # noqa: E402

EXE = os.path.join(paths.REPO, "build", "engine_synth.exe")
NAVMAP = os.environ.get("LTBF_NAVMAP", os.path.join(paths.WORK, "heightmap", "navmap.bin"))

# name, --character, air absorption
VARIANTS = [
    ("as FMOD", "fmod", False),
    ("no cone", "cone=0", False),
    ("no distortion", "distortion=0", False),
    ("no darkness", "darkness=0", False),
    ("no grit", "grit=0", False),
    ("air absorption", "fmod", True),
    ("clean", "clean", False),
    ("clean + air", "clean", True),
]


def write_float_wav(path, x, rate):
    data = np.repeat(x[:, None], 2, 1).astype("<f4").tobytes()
    head = (b"RIFF" + (36 + len(data)).to_bytes(4, "little") + b"WAVEfmt " + (16).to_bytes(4, "little")
            + (3).to_bytes(2, "little") + (2).to_bytes(2, "little") + rate.to_bytes(4, "little")
            + (rate * 8).to_bytes(4, "little") + (8).to_bytes(2, "little") + (32).to_bytes(2, "little")
            + b"data" + len(data).to_bytes(4, "little"))
    with open(path, "wb") as f:
        f.write(head + data)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run")
    ap.add_argument("--take", default="free_roam")
    ap.add_argument("--navmap", default=NAVMAP)
    ap.add_argument("--tour", type=float, default=10.0)
    args = ap.parse_args()
    stems = open(os.path.join(args.run, "stems.path")).read().strip()
    take = os.path.join(args.run, args.take)
    work = os.path.join(args.run, "variants")
    os.makedirs(work, exist_ok=True)

    event, rate = sv.read_float_wav(stems + "_wet.wav")
    event = event.mean(1)
    target = np.sqrt((event ** 2).mean())
    renders = []
    for k, (name, character, air) in enumerate(VARIANTS):
        prefix = os.path.join(work, f"v{k}")
        cmd = [EXE, stems, prefix, "--character", character, "--acoustics", args.navmap] + (["--air"] if air else [])
        out = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.strip().splitlines()
        x, _ = sv.read_float_wav(prefix + "_wet.wav")
        x = x.mean(1)
        gain = target / max(np.sqrt((x ** 2).mean()), 1e-9)
        x = x * gain
        write_float_wav(prefix + "_level.wav", x, rate)
        renders.append(x)
        print(f"{k + 1}. {name:15s} ({character}{', air' if air else ''}): {20 * np.log10(gain):+.1f} dB to the "
              f"event's level; {out[-1] if out else ''}")

    # the tour: each variant in turn
    n = min(len(r) for r in renders)
    tour = np.zeros(n, np.float32)
    seg = int(args.tour * rate)
    for i in range(0, n, seg):
        tour[i:i + seg] = renders[(i // seg) % len(renders)][i:i + seg]
    write_float_wav(os.path.join(work, "tour.wav"), tour, rate)

    # placed as the stems are on the video
    with open(take + ".wall") as f:
        wall = float(f.read())
    with open(take + ".offset") as f:
        game_at = float(f.read())
    info = json.load(open(stems + ".json"))
    coarse = (info["startWallOfDay"] - wall + 43200) % 86400 - 43200
    game, game_sr = sv.read_pcm_wav(take + ".wav")
    wet, _ = sv.read_float_wav(stems + "_wet.wav")
    r, lag = sv.refine(wet, rate, game, game_sr, coarse - game_at)
    at = coarse + lag

    tracks = [("tour", os.path.join(work, "tour.wav"))]
    tracks += [(name, os.path.join(work, f"v{k}_level.wav")) for k, (name, _, _) in enumerate(VARIANTS)]
    tracks += [("FMOD event as played", stems + "_wet.wav")]
    peak = max(np.abs(tour).max(), max(np.abs(x).max() for x in renders), np.abs(event).max(), 1e-6)
    gain = 10 ** (-1 / 20) / peak
    inputs = ["-i", take + ".mp4"]
    filters = []
    for k, (name, path) in enumerate(tracks):
        inputs += ["-i", path]
        filters.append(f"[{k + 1}:a]volume={gain:.4f},{sv.place('x', at)[3:]},apad[t{k}]")
    inputs += ["-i", take + ".wav"]
    filters.append(f"{sv.place(f'{len(tracks) + 1}:a', game_at)},apad[game]")
    out = take + "_variants.mp4"
    cmd = [paths.FFMPEG, "-hide_banner", "-loglevel", "error", "-y", *inputs, "-filter_complex", ";".join(filters),
           "-map", "0:v"]
    for k in range(len(tracks)):
        cmd += ["-map", f"[t{k}]"]
    cmd += ["-map", "[game]", "-c:v", "h264_nvenc", "-b:v", "1500k", "-maxrate", "1800k", "-bufsize", "3600k",
            "-c:a", "aac", "-b:a", "96k"]
    for k, (name, _) in enumerate(tracks + [("game recording", None)]):
        cmd += [f"-metadata:s:a:{k}", f"title={name}", f"-disposition:a:{k}", "default" if k == 0 else "0"]
    cmd += ["-shortest", out]
    subprocess.run(cmd, check=True)
    print(f"tour: {args.tour:g} s each, in the order above, from the stems' start ({at:+.1f} s in the video)")
    print("video:", out)


if __name__ == "__main__":
    sys.exit(main())
