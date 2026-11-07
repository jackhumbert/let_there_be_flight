"""BC3 (DXT5) texture data written straight, for scrape_effects.py: WolvenKit's import premultiplies colour by alpha (in
linear light), PNG or DDS alike, so a decal texture's soft edges came out darkened, and darkened again by the decal's
own alpha blend; thin lines, nearly all edge, read darker than the road. Here colour is kept whatever the alpha, the
mips average colour weighted by alpha (so transparent texels do not pull it toward black), and the blocks are packed
as the xbm stores them, to replace an imported texture's data in its CR2W JSON (replace_data)."""

import base64
import json

import numpy as np


def mip_chain(rgba, count):
    """count levels from rgba (H x W x 4, uint8), each half the last; colour averaged by alpha."""
    out = [rgba]
    a = rgba.astype(np.float64)
    for _ in range(count - 1):
        h, w = max(a.shape[0] // 2, 1), max(a.shape[1] // 2, 1)
        if a.shape[0] < 2 or a.shape[1] < 2:
            a = a[:1, :1]
            out.append(np.clip(a + 0.5, 0, 255).astype(np.uint8))
            continue
        b = a[: h * 2, : w * 2].reshape(h, 2, w, 2, 4)
        al = b[..., 3]
        wsum = al.sum(axis=(1, 3))
        rgb = (b[..., :3] * al[..., None]).sum(axis=(1, 3)) / np.maximum(wsum, 1e-6)[..., None]
        rgb = np.where(wsum[..., None] > 0, rgb, b[..., :3].mean(axis=(1, 3)))
        a = np.dstack([rgb, al.mean(axis=(1, 3))])
        out.append(np.clip(a + 0.5, 0, 255).astype(np.uint8))
    return out


def _blocks(rgba):
    h, w = rgba.shape[:2]
    ph, pw = max(4, (h + 3) // 4 * 4), max(4, (w + 3) // 4 * 4)
    img = np.pad(rgba, ((0, ph - h), (0, pw - w), (0, 0)), mode="edge")
    return img.reshape(ph // 4, 4, pw // 4, 4, 4).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 4).astype(np.int32)


def _to565(rgb):
    return ((rgb[..., 0] * 31 + 127) // 255 << 11) | ((rgb[..., 1] * 63 + 127) // 255 << 5) | ((rgb[..., 2] * 31 + 127) // 255)


def _from565(c):
    r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
    return np.stack([(r * 255 + 15) // 31, (g * 255 + 31) // 63, (b * 255 + 15) // 31], -1)


def encode(rgba):
    """One mip level (H x W x 4, uint8) as BC3 blocks, row by row."""
    px = _blocks(rgba)
    n = px.shape[0]
    # alpha: the block's range in 8 steps (a0 > a1); a flat block uses index 0
    al = px[..., 3]
    a0, a1 = al.max(1), al.min(1)
    flat = a0 == a1
    a1 = np.where(flat, np.maximum(a0 - 1, 0), a1)
    a0 = np.where(flat & (a0 == 0), 1, a0)
    steps = np.stack([a0, a1] + [((7 - k) * a0 + k * a1) // 7 for k in range(1, 7)], 1)  # codes 0..7
    aidx = np.abs(al[:, :, None] - steps[:, None, :]).argmin(2).astype(np.uint64)
    abits = np.zeros(n, np.uint64)
    for i in range(16):
        abits |= aidx[:, i] << np.uint64(3 * i)
    # colour: the block's bounding box ends, 4 steps
    rgb = px[..., :3]
    c0, c1 = _to565(rgb.max(1)), _to565(rgb.min(1))
    swap = c0 < c1
    c0, c1 = np.where(swap, c1, c0), np.where(swap, c0, c1)
    same = c0 == c1
    c0 = np.where(same & (c0 < 0xFFFF), c0 + 1, c0)
    c1 = np.where(same & (c0 == 0xFFFF), c1 - 1, c1)
    p0, p1 = _from565(c0), _from565(c1)
    pal = np.stack([p0, p1, (2 * p0 + p1) // 3, (p0 + 2 * p1) // 3], 1)
    cidx = ((rgb[:, :, None, :] - pal[:, None, :, :]) ** 2).sum(3).argmin(2).astype(np.uint32)
    cbits = np.zeros(n, np.uint32)
    for i in range(16):
        cbits |= cidx[:, i] << np.uint32(2 * i)
    out = np.zeros((n, 16), np.uint8)
    out[:, 0], out[:, 1] = a0, a1
    for k in range(6):
        out[:, 2 + k] = (abits >> np.uint64(8 * k)) & np.uint64(0xFF)
    out[:, 8], out[:, 9] = c0 & 0xFF, c0 >> 8
    out[:, 10], out[:, 11] = c1 & 0xFF, c1 >> 8
    for k in range(4):
        out[:, 12 + k] = (cbits >> np.uint32(8 * k)) & np.uint32(0xFF)
    return out.tobytes()


def replace_data(xbm_json, rgba):
    """Puts rgba (its mips made here) into an imported BC3 xbm's serialized JSON, at the header's placements."""
    data = json.loads(xbm_json.read_text(encoding="utf-8"))
    blob = data["Data"]["RootChunk"]["renderTextureResource"]["renderResourceBlobPC"]["Data"]
    header = blob["header"]
    infos = header["mipMapInfo"]
    levels = mip_chain(rgba, len(infos))
    buf = bytearray(header["textureInfo"]["textureDataSize"])
    for info, level in zip(infos, levels):
        place = info["placement"]
        enc = encode(level)
        if len(enc) != place["size"]:
            raise ValueError(f"mip {level.shape[:2]}: {len(enc)} bytes, the xbm has {place['size']}")
        buf[place["offset"] : place["offset"] + place["size"]] = enc
    blob["textureData"]["Bytes"] = base64.b64encode(bytes(buf)).decode()
    xbm_json.write_text(json.dumps(data, indent=2), encoding="utf-8")
