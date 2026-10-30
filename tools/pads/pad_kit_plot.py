"""Top and side scatter of a pad kit's parts (pad_kit.py output), coloured by part family.

  python pad_kit_plot.py kit.json -o kit.png
"""
import argparse
import json
import re

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402


def family(r):
    if r["type"] == "collisionActor":
        return "collision"
    m = r.get("mesh") or (r.get("inner") or {}).get("mesh") or ""
    name = m.split("\\")[-1]
    for pat, fam in (("railing|post", "railings"), ("catwalk", "catwalk"), ("canopy", "canopy"),
                     ("grating|floor", "deck"), ("landing_pad", "markings"), ("industrial", "industrial")):
        if re.search(pat, name):
            return fam
    return "other"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("kit")
    ap.add_argument("-o", "--out", required=True)
    args = ap.parse_args()
    parts = json.load(open(args.kit))["parts"]
    fams = sorted({family(p) for p in parts})
    colors = dict(zip(fams, plt.cm.tab10.colors))
    fig, (top, side) = plt.subplots(1, 2, figsize=(14, 6.5))
    for f in fams:
        pts = [p["local"] for p in parts if family(p) == f]
        kw = dict(s=6 if f == "collision" else 14, color=colors[f], label=f"{f} ({len(pts)})",
                  alpha=0.35 if f == "collision" else 0.9)
        top.scatter([p[0] for p in pts], [p[1] for p in pts], **kw)
        side.scatter([p[0] for p in pts], [p[2] for p in pts], **kw)
    top.set_title("top (pad frame, m)")
    top.set_aspect("equal")
    side.set_title("side: x against height (m)")
    side.set_aspect("equal")
    top.legend(fontsize=8, loc="upper left")
    for a in (top, side):
        a.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(args.out, dpi=110)


if __name__ == "__main__":
    main()
