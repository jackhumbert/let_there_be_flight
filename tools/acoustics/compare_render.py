"""Compare the plugin code's render (the acoustics target's <prefix>_dry.wav / _wet.wav, float) with
flyby.py's (dry.wav / wet.wav in its --out folder, scaled to peak): the echoes alone (wet - dry) on
each side, flyby.py's brought back to the recording's scale. Prints the waveform difference (for
information: the two move between probes a little differently, which shows in phase) and then what
is heard: the echoes' level per second in each octave band, side by side.

    python tools/acoustics/compare_render.py FLYBY_OUT_DIR PREFIX [--tol 1.0]

Exit 1 if any octave's 95th percentile level difference passes --tol dB."""

import argparse
import os
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import correlate


def read(path):
    sr, x = wavfile.read(path)
    if x.dtype == np.int16:
        x = x.astype(np.float64) / 32768
    return sr, x.astype(np.float64)


def db(x):
    return 10 * np.log10(x + 1e-20)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("flyby")
    ap.add_argument("prefix")
    ap.add_argument("--tol", type=float, default=1.0)
    args = ap.parse_args()
    sr, py_dry = read(os.path.join(args.flyby, "dry.wav"))
    _, py_wet = read(os.path.join(args.flyby, "wet.wav"))
    sr2, c_dry = read(args.prefix + "_dry.wav")
    _, c_wet = read(args.prefix + "_wet.wav")
    assert sr == sr2, f"sample rates {sr} and {sr2}"
    # line them up (an --audio-at rounded from flyby.py's print is a few samples out)
    mid, w, r = len(c_dry) // 2, sr * 2, sr // 10
    xc = correlate(py_dry[mid - w - r:mid + w + r, 0], c_dry[mid - w:mid + w, 0], mode="valid")
    shift = int(np.argmax(np.abs(xc))) - r
    if shift > 0:
        py_dry, py_wet = py_dry[shift:], py_wet[shift:]
    elif shift < 0:
        c_dry, c_wet = c_dry[-shift:], c_wet[-shift:]
    print(f"lined up: flyby.py's recording {shift} samples later")
    n = min(len(py_dry), len(c_dry))
    py_dry, py_wet, c_dry, c_wet = py_dry[:n], py_wet[:n], c_dry[:n], c_wet[:n]
    # flyby.py scaled everything by one factor (and rounded to 16 bits): find it from the dry sound
    scale = float(np.sum(py_dry * c_dry) / np.sum(c_dry * c_dry))
    lag = np.abs(py_dry / scale - c_dry).max()
    print(f"scale {scale:.4f}, dry sounds differ by at most {lag:.2e} (16 bit rounding: {1 / 32768 / scale:.2e})")
    py = (py_wet - py_dry) / scale
    c = c_wet - c_dry
    diff = c - py
    total = db(np.mean(diff ** 2)) - db(np.mean(py ** 2))
    level_py = db(np.mean(py ** 2) / np.mean(c_dry ** 2))
    level_c = db(np.mean(c ** 2) / np.mean(c_dry ** 2))
    sec = sr
    k = n // sec
    per = [db(np.mean(diff[i * sec:(i + 1) * sec] ** 2)) - db(np.mean(py[i * sec:(i + 1) * sec] ** 2) + 1e-12)
           for i in range(k)]
    worst = int(np.argmax(per))
    print(f"echoes against the dry sound: flyby.py {level_py:.1f} dB, plugin {level_c:.1f} dB")
    print(f"waveform difference against flyby.py's echoes: {total:.1f} dB overall, worst second {worst} s at "
          f"{per[worst]:.1f} dB (phase: a tenth of a sample of delay counts here)")
    # what is heard: each echo signal's level per second and octave band
    f, sp_py, sp_c = None, [], []
    for i in range(k):
        seg = slice(i * sec, (i + 1) * sec)
        win = np.hanning(sec)[:, None]
        sp_py.append(np.abs(np.fft.rfft(py[seg] * win, axis=0)) ** 2)
        sp_c.append(np.abs(np.fft.rfft(c[seg] * win, axis=0)) ** 2)
    f = np.fft.rfftfreq(sec, 1 / sr)
    rows = []
    for lo in (63, 125, 250, 500, 1000, 2000, 4000, 8000):
        band = (f >= lo) & (f < 2 * lo)
        e_py = np.array([s[band].sum() for s in sp_py])
        e_c = np.array([s[band].sum() for s in sp_c])
        # flyby.py's echoes come from two 16 bit files: their rounding (white, 1/12 lsb^2 each) in
        # this band, after undoing the scale
        floor16 = 2 * (1 / 32768) ** 2 / 12 / scale ** 2 * np.sum(np.hanning(sec) ** 2) * band.sum() * py.shape[1]
        # seconds where this band's echoes are within 40 dB of their loudest and 10 dB over that floor
        audible = (e_py > e_py.max() * 1e-4) & (e_py > 10 * floor16)
        if not audible.any():
            print(f"  {lo:5d} Hz octave: under flyby.py's 16 bit floor throughout")
            continue
        d = np.abs(db(e_c[audible]) - db(e_py[audible]))
        rows.append((lo, np.median(d), np.percentile(d, 95)))
        print(f"  {lo:5d} Hz octave: level difference median {rows[-1][1]:.2f} dB, p95 {rows[-1][2]:.2f} dB")
    p95 = max(r[2] for r in rows)
    print(f"band levels: worst p95 {p95:.2f} dB (tolerance {args.tol:.1f})")
    sys.exit(0 if p95 <= args.tol else 1)


if __name__ == "__main__":
    main()
