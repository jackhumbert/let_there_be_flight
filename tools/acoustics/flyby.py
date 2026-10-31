"""Hear a recorded flight with acoustics from the nav map: probes the space round the car along an
Autopilot telemetry file (probe.py), renders the run's own recording (--audio, a test run's
lib.Recording take) or else the FMOD project's engine and wind loops through it (render.py) and
writes, into --out:

  features.csv   probe numbers over time (floor, ceiling, walls, enclosure, mean free path)
  dry.wav        the source alone
  wet.wav        the source heard in the space
  ab.wav         alternating dry and wet every --ab seconds (dry first)
  flyby.png      the geometry, the reverb and the wet spectrogram, which shows the combs
  flyby.mp4      flyby.png with a cursor, over wet.wav (--video)
  game_wet.mp4, game_ab.mp4   the take's own footage over wet.wav / ab.wav (--video with --audio)

    python tools/acoustics/flyby.py TELEMETRY.csv --out DIR [--audio TAKE.wav [--audio-at s]]
        [--start 0] [--seconds 40] [--listener back,up] [--gain 1] [--video]

The listener is at the car (the soundListener component rides with V); --listener 6,2 puts it where
a third person camera would be. The nav map defaults to the local bake (LTBF_NAVMAP overrides)."""

import argparse
import csv
import datetime
import math
import os
import subprocess
import sys
import wave

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)

from common import paths  # noqa: E402
from common.navmap import NavMap  # noqa: E402
from probe import Probe  # noqa: E402
from render import acoustics, render  # noqa: E402

NAVMAP = os.environ.get("LTBF_NAVMAP", os.path.join(paths.WORK, "heightmap", "navmap.bin"))
ASSETS = os.path.join(paths.REPO, "src", "fmod_studio", "Assets")
PROBE_HZ = 15.0


def read_wav(path, mono=True):
    with wave.open(path) as w:
        sr, ch = w.getframerate(), w.getnchannels()
        x = np.frombuffer(w.readframes(w.getnframes()), "<i2").astype(np.float64) / 32768
    x = x.reshape(-1, ch)
    return (x.mean(1) if mono else x if ch == 2 else np.repeat(x, 2, 1)), sr


def align(telemetry_path, audio_path, rec, sr):
    """Seconds into the recording of the telemetry's t = 0. The clocks give it to a second or so (the
    telemetry is named for its local start time, the recording's .wall is its start in UTC seconds of
    day, .offset the audio's lag behind that); then the lag within 3 s where the recording's
    loudness follows the car's speed best."""
    base = os.path.splitext(audio_path)[0]
    with open(base + ".wall") as f:
        wall = float(f.read())
    with open(base + ".offset") as f:
        wall += float(f.read())
    stamp = datetime.datetime.strptime(os.path.basename(telemetry_path)[-19:-4], "%Y%m%d-%H%M%S").astimezone()
    utc = stamp.astimezone(datetime.timezone.utc)
    start = utc.hour * 3600 + utc.minute * 60 + utc.second
    coarse = (start - wall + 43200) % 86400 - 43200
    with open(telemetry_path) as f:
        rows = list(csv.DictReader(f))
    t = np.array([float(r["t"]) for r in rows])
    v = np.array([float(r["speed"]) for r in rows])
    hop = sr // 20
    m = rec.mean(1)
    k = len(m) // hop
    loud = 20 * np.log10(np.sqrt((m[:k * hop].reshape(k, hop) ** 2).mean(1)) + 1e-5)
    te = np.arange(k) / 20
    best = (-2.0, coarse)
    for lag in np.arange(-3, 3, 0.05):
        at = coarse + lag + t
        ok = (at >= 0) & (at < te[-1])
        if ok.sum() > 100:
            c = np.corrcoef(np.interp(at[ok], te, loud), v[ok])[0, 1]
            best = max(best, (c, coarse + lag))
    print(f"audio: telemetry starts {best[1]:.4f} s into the recording (clocks {coarse:.2f}, loudness ~ speed r={best[0]:.2f})")
    if best[0] < 0.4:
        print("audio: weak match, check it or pass --audio-at")
    return best[1]


def write_wav(path, x, sr):
    x = np.clip(x, -1, 1)
    with wave.open(path, "wb") as w:
        w.setnchannels(x.shape[1] if x.ndim > 1 else 1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes((x * 32767).astype("<i2").tobytes())


def one_pole(x, fc, sr):
    from scipy.signal import lfilter
    a = math.exp(-2 * math.pi * fc / sr)
    return lfilter([1 - a], [1, -a], x)


def source(speed, sr):
    """The car: the engine loop pitched up with speed, and wind growing with the square of it."""
    engine, esr = read_wav(os.path.join(ASSETS, "2077.wav"))
    noise, _ = read_wav(os.path.join(ASSETS, "white.wav"))
    rate = (0.85 + 0.45 * np.clip(speed / 70, 0, 1.3)) * esr / sr
    phase = np.cumsum(rate) % (len(engine) - 1)
    car = np.interp(phase, np.arange(len(engine)), engine)
    wind = np.resize(noise, len(speed))
    wind = one_pole(wind, 2500, sr) - one_pole(wind, 150, sr)
    wind *= 1.5 * np.clip(speed / 70, 0, 1.3) ** 2
    return 0.6 * car + wind


def telemetry(path, start, seconds):
    with open(path) as f:
        rows = [{k: float(v) for k, v in r.items()} for r in csv.DictReader(f)]
    t0 = rows[0]["t"] + start
    rows = [r for r in rows if r["t"] >= t0 and (seconds <= 0 or r["t"] < t0 + seconds)]
    for r in rows:
        r["t"] -= t0
    return rows


def probe_run(nav, rows):
    probe = Probe(nav)
    t = np.array([r["t"] for r in rows])
    feats = []
    for tp in np.arange(0, t[-1], 1 / PROBE_HZ):
        r = rows[int(np.searchsorted(t, tp))]
        pos = np.array([r["x"], r["y"], r["z"]])
        fwd = np.array([r["fwd_x"], r["fwd_y"], r["fwd_z"]])
        f = probe(pos, fwd, np.array([r["up_x"], r["up_y"], r["up_z"]]))
        f.update(t=tp, pos=pos, fwd=fwd, speed=r["speed"])
        feats.append(f)
    return feats


def plot(path, feats, par, wet, sr):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    t = np.array([f["t"] for f in feats])
    fig, ax = plt.subplots(3, 1, figsize=(12.8, 7.2), dpi=100, sharex=True,
                           gridspec_kw={"height_ratios": [2, 1, 2]})
    for key, colour in (("floor", "tab:brown"), ("ceiling", "tab:purple"), ("left", "tab:blue"), ("right", "tab:orange")):
        ax[0].plot(t, [min(f[key], 200) for f in feats], color=colour, label=key, lw=1)
    ax[0].set_ylabel("distance, m")
    ax[0].set_yscale("log")
    ax[0].set_ylim(1, 220)
    ax[0].legend(loc="upper right", ncol=4, fontsize=8)
    ts = np.arange(len(par["rt60"])) / sr
    ax[1].plot(ts[::441], par["rt60"][::441], color="tab:green", lw=1, label="RT60, s")
    ax[1].plot(ts[::441], 10 * par["wet"][::441], color="tab:red", lw=1, label="wet x10")
    ax[1].plot(t, [f["enclosure"] for f in feats], color="tab:gray", lw=1, label="enclosure")
    ax[1].legend(loc="upper right", ncol=3, fontsize=8)
    ax[2].specgram(wet.mean(1), NFFT=2048, Fs=sr, noverlap=1536, cmap="magma", vmin=-110)
    ax[2].set_ylim(0, 4000)
    ax[2].set_ylabel("wet, Hz")
    ax[2].set_xlabel("s")
    ax[2].set_xlim(0, t[-1])
    fig.tight_layout()
    fig.savefig(path)
    # the plot area in pixels, for the video's cursor
    x0 = ax[2].get_position().x0 * fig.bbox.width
    x1 = ax[2].get_position().x1 * fig.bbox.width
    plt.close(fig)
    return x0, x1, t[-1]


def video(out, png, wav, x0, x1, length):
    expr = f"{x0:.1f}+({x1 - x0:.1f})*t/{length:.3f}"
    subprocess.run([paths.FFMPEG, "-y", "-loglevel", "error", "-loop", "1", "-i", png, "-i", wav,
                    "-filter_complex", f"[0]format=yuv420p,drawbox=x='{expr}':y=0:w=2:h=ih:color=white@0.8:t=fill[v]",
                    "-map", "[v]", "-map", "1", "-r", "30", "-c:v", "libx264", "-c:a", "aac", "-b:a", "192k",
                    "-shortest", out], check=True)


def game_video(out, take, start, length, wav):
    """The run's own window recording from start s for length s, over wav."""
    subprocess.run([paths.FFMPEG, "-y", "-loglevel", "error", "-ss", f"{start:.3f}", "-t", f"{length:.3f}", "-i", take,
                    "-i", wav, "-map", "0:v", "-map", "1:a", "-c:v", "libx264", "-preset", "fast", "-b:v", "2500k",
                    "-c:a", "aac", "-b:a", "192k", "-shortest", out], check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("telemetry")
    ap.add_argument("--out", required=True)
    ap.add_argument("--navmap", default=NAVMAP)
    ap.add_argument("--start", type=float, default=0.0)
    ap.add_argument("--seconds", type=float, default=40.0, help="0: all")
    ap.add_argument("--listener", default="0,0", help="back,up metres from the car")
    ap.add_argument("--gain", type=float, default=1.0, help="scales every reflection and the reverb")
    ap.add_argument("--ab", type=float, default=4.0)
    ap.add_argument("--video", action="store_true")
    ap.add_argument("--audio", help="the run's recording (lib.Recording's <name>.wav, .wall and .offset beside it)")
    ap.add_argument("--audio-at", type=float, help="s into the recording of the telemetry's start (default: from the clocks)")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    rows = telemetry(args.telemetry, args.start, args.seconds)
    feats = probe_run(NavMap(args.navmap), rows)
    with open(os.path.join(args.out, "features.csv"), "w", newline="") as f:
        keys = ["t", "speed", "floor", "ceiling", "left", "right", "enclosure", "mean_free_path"]
        w = csv.writer(f)
        w.writerow(keys)
        for ft in feats:
            w.writerow([f"{ft[k]:.3f}" for k in keys])

    t_feat = np.array([f["t"] for f in feats])
    if args.audio:
        rec, sr = read_wav(args.audio, mono=False)
        at = args.audio_at if args.audio_at is not None else align(args.telemetry, args.audio, rec, sr)
        i0 = int(round((at + args.start) * sr))
        n = min(int(feats[-1]["t"] * sr), len(rec) - i0)
        if i0 < 0 or n <= 0:
            sys.exit(f"the recording does not cover the telemetry from {args.start} s (it starts at {at:.2f} s)")
        direct = rec[i0:i0 + n]
        dry = direct.mean(1)
    else:
        sr = 44100
        n = int(feats[-1]["t"] * sr)
        speed = np.interp(np.arange(n) / sr, t_feat, [f["speed"] for f in feats])
        dry = source(speed, sr)
        direct = np.stack([dry, dry], 1)
    back, up = (float(v) for v in args.listener.split(","))
    par = acoustics(feats, t_feat, n, sr, (back, up), gain=args.gain)
    wet, parts = render(dry, sr, par, direct)
    rms = math.sqrt(np.mean(dry ** 2))
    print("level re dry: " + ", ".join(f"{k} {20 * math.log10(math.sqrt(np.mean(v ** 2)) / rms + 1e-9):.1f} dB"
                                       for k, v in parts.items()))
    scale = 0.9 / max(np.abs(wet).max(), 1e-9)
    dry2 = direct * scale
    wet *= scale
    write_wav(os.path.join(args.out, "dry.wav"), dry2, sr)
    write_wav(os.path.join(args.out, "wet.wav"), wet, sr)
    seg = (np.arange(n) / sr // args.ab).astype(np.int64) % 2 == 1
    write_wav(os.path.join(args.out, "ab.wav"), np.where(seg[:, None], wet, dry2), sr)
    x0, x1, length = plot(os.path.join(args.out, "flyby.png"), feats, par, wet, sr)
    if args.video:
        video(os.path.join(args.out, "flyby.mp4"), os.path.join(args.out, "flyby.png"),
              os.path.join(args.out, "wet.wav"), x0, x1, length)
        take = os.path.splitext(args.audio)[0] + ".mp4" if args.audio else ""
        if os.path.exists(take):
            with open(os.path.splitext(args.audio)[0] + ".offset") as f:
                lag = float(f.read())
            for name in ("wet", "ab"):
                game_video(os.path.join(args.out, f"game_{name}.mp4"), take, at + args.start + lag, n / sr,
                           os.path.join(args.out, f"{name}.wav"))
    print(f"{len(feats)} probes, {n / sr:.1f} s -> {args.out}")


if __name__ == "__main__":
    main()
