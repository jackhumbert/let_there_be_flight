"""Draw an AUTOPILOT logo in the style of vanilla's AUTODRIVE logo.

The vanilla logo (base\\gameplay\\gui\\widgets\\autodrive\\autodrive.xbm, part "logo", 210 x 34 px of
the 512 px texture) is a bold "AUTO" and a thin italic "DRIVE" whose letters share strokes (the D
has no stem of its own, it leans on the O). This keeps the vanilla "AUTO" pixels and draws "PILOT"
in the thin style: 4 px strokes, italic stems, the P leaning on the O like the D does.

  python tools/icons/autopilot_logo.py <vanilla png dir> <out dir>

<vanilla png dir> holds vanilla's autodrive.png, autodrive_1080p.png and autodrive_720p.png (the
three textures of autodrive.inkatlas, exported with WolvenKit `export --uext png`). Writes
autopilot_logo.png, autopilot_logo_1080p.png, autopilot_logo_720p.png: copies of those textures
with the logo rectangle repainted, so they encode exactly like the originals (same sizes, same
premultiplied white, RGB = alpha, peaking at vanilla's 229) and vanilla's atlas parts still fit;
plus preview.png (vanilla above ours). Then:
  python tools/icons/build_icon_atlas.py --icon autopilot --pngs <out dir> --work <scratch dir>
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

K = 8              # supersampling
W, H = 210, 34     # logo rectangle in the 512 px texture
AUTO_END = 102     # vanilla columns kept: the bold AUTO (its O ends at x 101)
TOP, BOTTOM = 1.0, 33.0
BAR = 3.7          # bar thickness (vanilla: about 3.7 px)
STEM = 4.0         # stem width (vanilla I: 4.0 px)
SLANT = 0.16       # x shift per px of height (vanilla I: 5 px over 32)
MID_Y = 17.0


def shear(x, y):
    return x + SLANT * (MID_Y - y)


class Pen:
    def __init__(self):
        self.img = Image.new("L", (W * K, H * K), 0)
        self.d = ImageDraw.Draw(self.img)

    def poly(self, pts):
        self.d.polygon([(shear(x, y) * K, y * K) for x, y in pts], fill=255)

    def bar(self, x0, x1, yc):
        h = BAR / 2
        self.poly([(x0, yc - h), (x1, yc - h), (x1, yc + h), (x0, yc + h)])

    def stem(self, xc, y0=TOP, y1=BOTTOM):
        h = STEM / 2
        self.poly([(xc - h, y0), (xc + h, y0), (xc + h, y1), (xc - h, y1)])

    def arc(self, cx, cy, rx, ry, a0, a1, steps=48):
        """A band of thickness ~BAR along an elliptic arc (angles in degrees, 0 = +x, 90 = +y down)."""
        outer, inner = [], []
        for i in range(steps + 1):
            a = math.radians(a0 + (a1 - a0) * i / steps)
            c, s = math.cos(a), math.sin(a)
            # thickness: STEM where the band runs vertically, BAR where it runs horizontally
            t = (STEM * abs(c) + BAR * abs(s)) / 2
            outer.append((cx + (rx + t) * c, cy + (ry + t) * s))
            inner.append((cx + (rx - t) * c, cy + (ry - t) * s))
        self.poly(outer + inner[::-1])


def draw_pilot(pen):
    yt, yb = TOP + BAR / 2, BOTTOM - BAR / 2       # bar centres
    # P: top bar, bowl, middle bar back to the O (like vanilla's D, which has no stem)
    x = 104.0
    ym = 18.5
    r = (ym - yt) / 2
    pen.bar(x, x + 10, yt)
    pen.arc(x + 10, yt + r, r + 1.5, r, -90, 90)
    pen.bar(AUTO_END - 3, x + 10, ym)
    # I
    x = 132.0
    pen.stem(x)
    # L
    x = 141.0
    pen.stem(x)
    pen.bar(x - STEM / 2, x + 14, yb)
    # O: rounded italic stadium
    x0, x1 = 161.0, 184.0
    r = (yb - yt) / 2
    rx = 7.5
    pen.bar(x0 + rx, x1 - rx, yt)
    pen.bar(x0 + rx, x1 - rx, yb)
    pen.arc(x1 - rx, yt + r, rx, r, -90, 90)
    pen.arc(x0 + rx, yt + r, rx, r, 90, 270)
    # T
    x0, x1 = 186.0, 208.0
    pen.bar(x0, x1, yt)
    pen.stem((x0 + x1) / 2 - 1.0)


# vanilla texture, its logo part's size (autodrive.inkatlas), our texture
SLOTS = (("autodrive", (210, 34), "autopilot_logo"),
         ("autodrive_1080p", (105, 17), "autopilot_logo_1080p"),
         ("autodrive_720p", (70, 12), "autopilot_logo_720p"))
PEAK = 229  # vanilla's logo pixels top out here


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)

    pen = Pen()
    draw_pilot(pen)
    pilot = pen.img.resize((W, H), Image.LANCZOS).point(lambda v: v * PEAK // 255)
    previews = []
    for vanilla_name, (w, h), name in SLOTS:
        alpha = Image.open(src / f"{vanilla_name}.png").convert("RGBA").split()[3]
        old = alpha.crop((0, 0, w, h))
        # vanilla's own AUTO at this resolution, our PILOT scaled down from the 4K drawing
        logo = Image.new("L", (w, h), 0)
        logo.paste(old.crop((0, 0, round(AUTO_END * w / W), h)), (0, 0))
        logo = ImageChops.lighter(logo, pilot if (w, h) == (W, H) else pilot.resize((w, h), Image.LANCZOS))
        alpha.paste(logo, (0, 0))
        Image.merge("RGBA", (alpha, alpha, alpha, alpha)).save(out / f"{name}.png")
        print(name, alpha.size, "logo", (w, h))
        if (w, h) == (W, H):
            previews = [old, logo]

    # preview: vanilla above ours, HUD cyan on a dark background, 4x
    s = 4
    prev = Image.new("RGB", (W * s, (H * 2 + 8) * s), (16, 20, 26))
    for i, a in enumerate(previews):
        prev.paste((94, 246, 255), (0, i * (H + 8) * s), mask=a.resize((W * s, H * s), Image.LANCZOS))
    prev.save(out / "preview.png")


if __name__ == "__main__":
    main()
