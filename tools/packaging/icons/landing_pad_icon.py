"""Draw the landing pad map icon in the style of the game's mappin_icons.inkatlas.

The vanilla icons are flat white silhouettes on transparency in a 64 x 64 cell (32 x 32 and
22 x 22 in the 1080p / 720p textures), filling most of the cell, anti-aliased, no outline; the
map and minimap tint them. This one is the helipad mark: a ring with a bold H.

Writes landing_pad_icon_64.png, _32.png and _22.png (and a preview sheet next to the vanilla
icons when --compare points at an exported mappin_icons.png) into the output folder.

Usage: python landing_pad_icon.py -o <dir> [--compare mappin_icons.png]
"""

import argparse
import os

from PIL import Image, ImageDraw

SS = 16  # supersampling factor

# geometry in 64-px cell units
RING_OUTER = 30.0   # radius
RING_WIDTH = 4.5
H_HEIGHT = 28.0
H_WIDTH = 25.0
H_STEM = 6.5        # stem width
H_BAR = 5.5         # crossbar height


def draw(size, ring_width=RING_WIDTH, stem=H_STEM, bar=H_BAR):
    """The icon at `size` px: drawn at 64 * SS, scaled down."""
    big = 64 * SS
    img = Image.new("L", (big, big), 0)
    d = ImageDraw.Draw(img)
    c = big / 2
    s = SS
    ro = RING_OUTER * s
    ri = (RING_OUTER - ring_width) * s
    d.ellipse((c - ro, c - ro, c + ro, c + ro), fill=255)
    d.ellipse((c - ri, c - ri, c + ri, c + ri), fill=0)
    hw, hh = H_WIDTH * s / 2, H_HEIGHT * s / 2
    st, br = stem * s, bar * s
    d.rectangle((c - hw, c - hh, c - hw + st, c + hh), fill=255)
    d.rectangle((c + hw - st, c - hh, c + hw, c + hh), fill=255)
    d.rectangle((c - hw, c - br / 2, c + hw, c + br / 2), fill=255)
    alpha = img.resize((size, size), Image.LANCZOS)
    # premultiplied, like the vanilla atlas (edge pixels there have RGB <= alpha): the UI blend
    # adds the RGB, so white (255, 255, 255, 0) drew a solid white square and white edge pixels
    # drew the anti-aliasing away (a hard, bold outline)
    return Image.merge("RGBA", (alpha, alpha, alpha, alpha))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--compare", help="exported vanilla mappin_icons.png (4K slot) for a side-by-side sheet")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    # the 22 px one gets slightly heavier strokes so the ring survives the downscale
    icons = {64: draw(64), 32: draw(32), 22: draw(22, ring_width=5.0, stem=7.0, bar=6.0)}
    for size, im in icons.items():
        im.save(os.path.join(args.out, f"landing_pad_icon_{size}.png"))
    if args.compare:
        ref = Image.open(args.compare).convert("RGBA")
        cells = [ref.crop(b) for b in ((594, 200, 658, 264), (330, 2, 394, 66), (0, 662, 64, 726), (990, 727, 1054, 791))]
        sheet = Image.new("RGBA", (64 * 6 + 70, 64 * 2 + 30), (20, 24, 30, 255))
        for i, cell in enumerate(cells + [icons[64]]):
            sheet.alpha_composite(cell, (10 + i * 74, 10))
        # at the sizes the game uses, blended premultiplied (out = rgb * tint + bg * (1 - a)) as the
        # UI does, next to the vanilla vehicle icon at the same sizes
        bg = (20, 24, 30)
        for i, (size, cell) in enumerate(((32, icons[32]), (22, icons[22]),
                                          (32, cells[0].resize((32, 32), Image.LANCZOS)),
                                          (22, cells[0].resize((22, 22), Image.LANCZOS)))):
            out = Image.new("RGBA", cell.size)
            for x in range(cell.width):
                for y in range(cell.height):
                    r, g, b, a = cell.getpixel((x, y))
                    out.putpixel((x, y), tuple(min(255, round(c + k * (1 - a / 255))) for c, k in zip((r, g, b), bg)) + (255,))
            sheet.alpha_composite(out, (10 + i * 74, 84))
        sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
        sheet.save(os.path.join(args.out, "landing_pad_icon_compare.png"))


if __name__ == "__main__":
    main()
