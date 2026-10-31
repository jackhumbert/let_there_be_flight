"""The car's sound as heard in the space round it (probe.py's numbers over time): offline, so every
signal is a whole array and feedback is filled block by block, each block shorter than the
shortest delay it reads back.

  pair()   two facing surfaces (floor and ceiling, left and right wall): one echo off each, then
           cross feedback between them, the flutter that rings at c / 2w between parallel walls.
           The floor alone is the comb that sweeps as the car climbs or dives.
  fdn()    late reverb, an 8 line feedback delay network: level from how enclosed the car is,
           decay (RT60) from the mean free path and how much of the sky is open.

Delays follow the geometry sample by sample (linear interpolation), so a reflection carries its own
Doppler shift; the rate is capped so a ray sliding off a building edge glides instead of jumping."""

import math

import numpy as np

from probe import C

BLOCK = 128
MIN_DELAY = 0.004  # s, longer than a block


def read(buf, pos):
    """buf at fractional sample positions, a three tap smear (air and surface absorption)."""
    pos = np.clip(pos, 1, len(buf) - 2)
    i = pos.astype(np.int64)
    f = pos - i
    s = buf[i] * (1 - f) + buf[i + 1] * f
    return 0.5 * s + 0.25 * (buf[i - 1] * (1 - f) + buf[i] * f) + 0.25 * (buf[i + 1] * (1 - f) + buf[i + 2] * f)


def slew(x, sr, rate):
    """x (delay in seconds per sample) changing by at most rate seconds per second."""
    out = np.empty_like(x)
    out[0] = x[0]
    step = rate / sr
    for i in range(1, len(x)):  # ~2 M samples at most for a minute, fine offline
        out[i] = out[i - 1] + min(step, max(-step, x[i] - out[i - 1]))
    return out


def image_delay(listener, source, normal, distance):
    """Extra path (m) and spreading gain of the echo off a plane distance from the source along
    normal, heard at listener: the image source method."""
    image = source + 2 * distance[:, None] * normal
    direct = np.linalg.norm(listener - source, axis=1)
    via = np.linalg.norm(listener - image, axis=1)
    return via - direct, direct, via


def pair(src, sr, d_a, d_b, g_a, g_b, fb, rate=0.3):
    """Echoes off surfaces A and B (per-sample delays d_a, d_b in seconds, gains g_a, g_b) with
    feedback fb from each into the other (the second bounce takes the other's round trip)."""
    n = len(src)
    a, b = np.zeros(n), np.zeros(n)
    d_a = slew(np.maximum(d_a, MIN_DELAY), sr, rate) * sr
    d_b = slew(np.maximum(d_b, MIN_DELAY), sr, rate) * sr
    idx = np.arange(n, dtype=np.float64)
    for i0 in range(0, n, BLOCK):
        s = slice(i0, min(n, i0 + BLOCK))
        a[s] = g_a[s] * read(src, idx[s] - d_a[s]) + fb[s] * read(b, idx[s] - d_a[s])
        b[s] = g_b[s] * read(src, idx[s] - d_b[s]) + fb[s] * read(a, idx[s] - d_b[s])
    return a, b


FDN_MS = np.array([29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.1, 73.3])


def fdn(src, sr, rt60):
    """Stereo late reverb of src, with a per-sample RT60 (s)."""
    n = len(src)
    d = np.round(FDN_MS * 1e-3 * sr).astype(np.int64)
    lines = np.zeros((8, n))
    h = np.eye(8) - 2.0 / 8
    for i0 in range(0, n, BLOCK):
        i1 = min(n, i0 + BLOCK)
        idx = np.arange(i0, i1)
        back = np.stack([lines[j, np.maximum(idx - d[j], 0)] for j in range(8)])
        g = 10 ** (-3 * d[:, None] / (np.maximum(rt60[i0], 0.05) * sr))
        lines[:, i0:i1] = g * (h @ back) + src[i0:i1][None, :] / math.sqrt(8)
    return lines[0::2].sum(0) * 0.5, lines[1::2].sum(0) * 0.5


def acoustics(feat, t_feat, n, sr, listener_offset=(0.0, 0.0), r_wall=0.8, r_ground=0.7, gain=1.0):
    """Per-sample render parameters from probe features (list of dicts at times t_feat): the
    listener sits listener_offset (back, up) metres from the car, which is the source."""
    t = np.arange(n) / sr

    def track(key, cap=250.0):
        v = np.array([min(f[key], cap) for f in feat])
        return np.interp(t, t_feat, v)

    pos = np.stack([np.interp(t, t_feat, [f["pos"][k] for f in feat]) for k in range(3)], 1)
    fwd = np.stack([np.interp(t, t_feat, [f["fwd"][k] for f in feat]) for k in range(3)], 1)
    right = np.stack([np.interp(t, t_feat, [f["right_dir"][k] for f in feat]) for k in range(3)], 1)
    fwd /= np.linalg.norm(fwd, axis=1, keepdims=True)
    listener = pos - listener_offset[0] * fwd + np.array([0, 0, listener_offset[1]])
    up = np.array([0.0, 0.0, 1.0])
    out = {}
    for name, normal, r, opposite in (("floor", -up, r_ground, "ceiling"), ("ceiling", up, r_wall, "floor"),
                                       ("left", None, r_wall, "right"), ("right", None, r_wall, "left")):
        dist = track(name)
        nrm = normal if normal is not None else (right if name == "right" else -right)
        nrm = np.broadcast_to(nrm, pos.shape)
        extra, direct, via = image_delay(listener, pos, nrm, dist)
        # spreading relative to the dry sound heard at a few metres; nothing beyond the probe range
        g = r * gain * (direct + 3.0) / (via + 3.0) * (dist < 249)
        out[name] = (extra / C, g)
    # flutter only where both sides are there, fading out between 60 and 120 m of separation
    for a, b in (("floor", "ceiling"), ("left", "right")):
        w = track(a) + track(b)
        out[a + "_fb"] = 0.5 * math.sqrt(r_wall * r_ground if a == "floor" else r_wall) * np.clip((120 - w) / 60, 0, 1)
    enc = track("enclosure", 1.0)
    mfp = track("mean_free_path", 200.0)
    escape = 1 - enc
    # Eyring with mean free path = 4V/S; open sky absorbs everything, and the city's surfaces a lot
    # (glass and concrete reflect, but a street scatters most energy up and out)
    alpha = escape + (1 - escape) * 0.5
    out["rt60"] = np.clip(0.04 * mfp / -np.log(1 - np.minimum(alpha, 0.99)), 0.2, 2.0)
    # the lower half of the sphere is nearly always ground: only enclosure above that is room
    out["wet"] = 0.15 * gain * np.clip((enc - 0.5) / 0.5, 0, 1)
    return out


def render(dry, sr, par, direct=None):
    """Stereo (n, 2) of the dry mono signal heard in the space described by par (acoustics()), and
    each part on its own (mono) for levels. direct (n, 2) is the dry sound as heard (a stereo
    recording), dry itself in both channels if None."""
    fl, fl_g = par["floor"]
    ce, ce_g = par["ceiling"]
    le, le_g = par["left"]
    ri, ri_g = par["right"]
    floor, ceil = pair(dry, sr, fl, ce, fl_g, ce_g, par["floor_fb"])
    left, right = pair(dry, sr, le, ri, le_g, ri_g, par["left_fb"])
    rev_l, rev_r = fdn(dry * par["wet"], sr, par["rt60"])
    mid = floor + ceil
    out = np.stack([mid + 0.9 * left + 0.3 * right + rev_l, mid + 0.3 * left + 0.9 * right + rev_r], 1)
    out += np.stack([dry, dry], 1) if direct is None else direct
    return out, {"floor": floor, "ceiling": ceil, "walls": left + right, "reverb": rev_l + rev_r}
