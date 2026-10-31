"""The engine synth (tools/acoustics/engine_synth.cpp) against the FMOD event it stands in for, over one
run's stems: the FMOD thrusters (the stems' dry track) and the synth's voices rendered from the same
parameters (engine_synth --voices --character fmod: the event's character, not the clean default),
on one timeline.

    python tools/acoustics/engine_fit.py <stems base> <synth prefix> [--fit]

Prints, for each control held (from the parameter log: |axis| > 0.5, brake > 0.5, and at rest), the
level (dB), brightness (spectral centroid, Hz) and lowest strong partial (Hz) of both; and the
correlation of their loudness over the run. --fit solves each voice's level (non-negative least squares
on power in 50 ms frames) and prints the --tune line that would apply it.
"""

import argparse
import csv
import sys

import numpy as np
from scipy.optimize import nnls

VOICES = ["surge", "lift", "pitch", "roll", "sway", "air", "brake", "damage", "scrape"]
HOP = 0.05


def read_float_wav(path):
    with open(path, "rb") as f:
        head = f.read(44)
        channels = int.from_bytes(head[22:24], "little")
        rate = int.from_bytes(head[24:28], "little")
        x = np.frombuffer(f.read(), "<f4")
    x = x[: len(x) // channels * channels].reshape(-1, channels)
    return x.mean(1).astype(np.float64), rate


def frames_power(x, sr):
    hop = int(sr * HOP)
    k = len(x) // hop
    return (x[: k * hop].reshape(k, hop) ** 2).mean(1)


def describe(x, sr):
    """level dB, centroid Hz, lowest strong partial Hz"""
    if len(x) < sr // 4:
        return None
    level = 10 * np.log10((x ** 2).mean() + 1e-12)
    n = 1 << int(np.log2(min(len(x), sr * 2)))
    spec = np.abs(np.fft.rfft(x[:n] * np.hanning(n)))
    f = np.fft.rfftfreq(n, 1 / sr)
    centroid = (spec * f).sum() / (spec.sum() + 1e-12)
    band = (f > 25) & (f < 400)
    s = spec[band]
    strong = np.where(s > 0.3 * s.max())[0]
    low = f[band][strong[0]] if len(strong) else 0
    return level, centroid, low


def segments(params, start, sr, n):
    """(name, mask over samples) for each control held, from the parameter log's first thruster"""
    rows = list(csv.reader(open(params)))
    head, rows = rows[0], rows[1:]
    first = rows[0][1]
    rows = [r for r in rows if r[1] == first]
    col = {name: i for i, name in enumerate(head) if i >= 7}
    t = np.array([float(r[0]) for r in rows]) - start
    def series(name):
        return np.array([float(r[col[name]]) for r in rows])
    idx = np.clip((t * sr).astype(int), 0, n)
    out = []
    tests = [(f"{a} {'+' if s > 0 else '-'}", a, s) for a in ("surge", "lift", "pitch", "roll", "sway", "yaw") for s in (1, -1)]
    for name, axis, sign in tests:
        v = series(axis) * sign
        out.append((name, v > 0.5))
    out.append(("brake", series("brake") > 0.5))
    speed = series("speed")
    quiet = np.ones(len(rows), bool)
    for a in ("surge", "lift", "pitch", "roll", "sway", "yaw", "brake"):
        quiet &= np.abs(series(a)) < 0.15
    out.append(("rest, slow", quiet & (speed < 10)))
    out.append(("rest, fast", quiet & (speed > 40)))
    masks = []
    for name, on in out:
        m = np.zeros(n, bool)
        for i in np.where(on)[0][:-1]:
            m[idx[i]:idx[i + 1]] = True
        masks.append((name, m))
    return masks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("stems")
    ap.add_argument("synth")
    ap.add_argument("--fit", action="store_true")
    args = ap.parse_args()
    fmod, sr = read_float_wav(args.stems + "_dry.wav")
    synth, _ = read_float_wav(args.synth + ".wav")
    n = min(len(fmod), len(synth))
    fmod, synth = fmod[:n], synth[:n]
    import json
    start = json.load(open(args.stems + ".json"))["startWall"]

    pf, ps = frames_power(fmod, sr), frames_power(synth, sr)
    live = pf > 1e-7
    r = np.corrcoef(10 * np.log10(pf[live] + 1e-12), 10 * np.log10(ps[live] + 1e-12))[0, 1]
    print(f"{n / sr:.1f} s; loudness correlation (dB, 50 ms) {r:.2f}; level FMOD {10 * np.log10(pf.mean()):.1f} dB, "
          f"synth {10 * np.log10(ps.mean()):.1f} dB")
    print(f"{'held':12s} {'s':>5s} | {'FMOD dB':>8s} {'centroid':>8s} {'low':>5s} | {'synth dB':>8s} {'centroid':>8s} {'low':>5s}")
    for name, m in segments(args.stems + "_params.csv", start, sr, n):
        if m.sum() < sr // 4:
            continue
        a, b = describe(fmod[m], sr), describe(synth[m], sr)
        print(f"{name:12s} {m.sum() / sr:5.1f} | {a[0]:8.1f} {a[1]:8.0f} {a[2]:5.0f} | {b[0]:8.1f} {b[1]:8.0f} {b[2]:5.0f}")

    if args.fit:
        parts = []
        for v in VOICES:
            x, _ = read_float_wav(f"{args.synth}_{v}.wav")
            parts.append(frames_power(x[:n], sr))
        A = np.stack(parts, 1)
        used = A.max(0) > 1e-12
        g = np.zeros(len(VOICES))
        g[used], _ = nnls(A[:, used], pf[: len(A)])
        fitted = A @ g
        r2 = np.corrcoef(10 * np.log10(pf[live] + 1e-12), 10 * np.log10(fitted[live] + 1e-12))[0, 1]
        db = {v: 10 * np.log10(gv) for v, gv in zip(VOICES, g) if gv > 0}
        print(f"fit: loudness correlation {r2:.2f}; each voice's change (dB):",
              ", ".join(f"{v} {d:+.1f}" for v, d in db.items()),
              "; unused:", ", ".join(v for v, gv, u in zip(VOICES, g, used) if u and gv == 0) or "none")
        print("--tune " + ",".join(f"{v}={d:.1f}" for v, d in db.items()))


if __name__ == "__main__":
    sys.exit(main())
