"""Compare the echoes as FMOD played them (two acoustics_fmod runs of the same take, echoes on and
off: on minus off) with the acoustics target's offline render of the same run (<prefix>_wet.wav
minus <prefix>_dry.wav). Prints:

  - where the take starts in each recording and the bus's level against the take
  - with --off a,b (the on run's own off window): on minus off there, which is 0 if the DSP passes
    the sound through untouched, and the echo level in 10 ms steps round both switches (the fades)
  - the echoes' level against the dry sound, and per octave per second, FMOD minus offline

    python tools/acoustics/compare_fmod.py ON.wav OFF.wav PREFIX [--off a,b]

The two runs differ from the offline one in timing only (Params arrive with the probe, 20 Hz, not
ahead of it), unless the bus's limiter is driven: play the take with acoustics_fmod --volume 0.05."""

import argparse

import numpy as np
from scipy.io import wavfile
from scipy.signal import correlate


def read(path):
    sr, x = wavfile.read(path)
    return sr, (x / 32768 if x.dtype == np.int16 else x).astype(np.float64)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("on")
    ap.add_argument("off")
    ap.add_argument("prefix")
    ap.add_argument("--off-window", "--off", dest="window", help="a,b: seconds the on run had the echoes off")
    args = ap.parse_args()
    sr, on = read(args.on)
    _, off = read(args.off)
    _, cd = read(args.prefix + "_dry.wav")
    _, cw = read(args.prefix + "_wet.wav")

    def start(x):
        w, a = sr * 4, sr * 10
        xc = correlate(x[: a + w + 3 * sr, 0], cd[a:a + w, 0], mode="valid")
        return int(np.argmax(np.abs(xc))) - a

    s_on, s_off = start(on), start(off)
    on, off = on[s_on:], off[s_off:]
    n = min(len(on), len(off), len(cd))
    on, off, cd, cw = on[:n], off[:n], cd[:n], cw[:n]
    ef, ec = on - off, cw - cd
    g = np.sqrt(np.mean(off ** 2) / np.mean(cd ** 2))
    print(f"take starts {s_on} / {s_off} samples into the recordings; bus level against the take {20 * np.log10(g):.1f} dB")

    def lvl(x, a, b):
        return 10 * np.log10(np.mean(x[int(a * sr):int(b * sr)] ** 2) + 1e-30)

    keep = np.ones(n, bool)
    if args.window:
        a, b = (float(v) for v in args.window.split(","))
        print(f"off window: on minus off {lvl(ef, a + 0.2, b - 0.2) - lvl(off, a + 0.2, b - 0.2):.1f} dB re dry (untouched: very low)")
        for t0 in (a, b):
            print(f"  echo level round {t0:.1f} s, 10 ms steps, dB re dry:",
                  " ".join(f"{lvl(ef, t, t + 0.01) - lvl(off, t, t + 0.01):.0f}" for t in np.arange(t0 - 0.05, t0 + 0.12, 0.01)))
        keep[int((a - 1) * sr):int((b + 1) * sr)] = False
    print(f"echoes re dry: FMOD {10 * np.log10(np.sum(ef[keep] ** 2) / np.sum(off[keep] ** 2)):.1f} dB, "
          f"offline {10 * np.log10(np.sum(ec[keep] ** 2) / np.sum(cd[keep] ** 2)):.1f} dB")
    fr = np.fft.rfftfreq(sr, 1 / sr)
    win = np.hanning(sr)[:, None]
    seconds = [i for i in range(1, n // sr) if keep[i * sr:(i + 1) * sr].all()]
    for lo in (125, 250, 500, 1000, 2000, 4000, 8000):
        band = (fr >= lo) & (fr < 2 * lo)
        d = []
        for i in seconds:
            s = slice(i * sr, (i + 1) * sr)
            A = (np.abs(np.fft.rfft(ef[s] * win, axis=0)) ** 2)[band].sum() / g ** 2
            B = (np.abs(np.fft.rfft(ec[s] * win, axis=0)) ** 2)[band].sum()
            d.append(10 * np.log10(A / B))
        d = np.array(d)
        print(f"  {lo:5d} Hz octave, FMOD minus offline per second: median {np.median(d):+.2f} dB, "
              f"p5 {np.percentile(d, 5):+.2f}, p95 {np.percentile(d, 95):+.2f}")


if __name__ == "__main__":
    main()
