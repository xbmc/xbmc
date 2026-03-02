#!/usr/bin/env python3
"""PGS (.sup) inspector.

Goal: make it easy to answer "which palette indices/colors are actually used by this subtitle bitmap?"

- Parses Blu-ray SUP packets ("PG" packets).
- Extracts palette segments (YCbCr + alpha/transparency) and object segments (RLE index bitmaps).
- Extracts presentation segments (which objects are shown when and which palette_id they reference).
- Can dump per-event summaries and write bitmap images:
  - PGM (palette index map)
  - PPM (RGB preview, optional alpha applied as simple linear blend on black)

No external dependencies.

Notes:
- SUP packet timestamps are 32-bit PTS in 90kHz ticks and may wrap; for inspection we mainly use event order.
- PGS palette entries are stored as Y, Cr, Cb, Alpha where Alpha is TRANSPARENCY (0=opaque .. 255=transparent).
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Optional, Tuple

try:
    from PIL import Image  # type: ignore
except Exception:  # pragma: no cover
    Image = None


PALETTE_SEGMENT = 0x14
OBJECT_SEGMENT = 0x15
PRESENTATION_SEGMENT = 0x16
WINDOW_SEGMENT = 0x17
DISPLAY_SEGMENT = 0x80


@dataclass(frozen=True)
class PaletteEntry:
    idx: int
    y: int
    cb: int
    cr: int
    a_transparency: int


@dataclass
class Palette:
    palette_id: int
    version: int
    entries: Dict[int, PaletteEntry]


@dataclass
class ObjectBitmap:
    obj_id: int
    width: int
    height: int
    index_bitmap: bytes  # width*height indices


@dataclass
class _ObjectAssembly:
    obj_id: int
    obj_ver: int
    total_len: int
    width: Optional[int]
    height: Optional[int]
    data: bytearray


@dataclass
class PresentedObject:
    obj_id: int
    win_id: int
    comp_flag: int
    x: int
    y: int
    crop: Optional[Tuple[int, int, int, int]]


@dataclass
class Presentation:
    video_w: int
    video_h: int
    palette_id: int
    objects: List[PresentedObject]


@dataclass
class Event:
    order: int
    pts_90k: int
    packet_index: int
    presentation: Presentation


def _clamp8(x: float) -> int:
    if x < 0:
        return 0
    if x > 255:
        return 255
    return int(round(x))


def _clampf(x: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return lo if x < lo else hi if x > hi else x


def _srgb_to_linear(u: float) -> float:
    u = _clampf(u)
    if u <= 0.04045:
        return u / 12.92
    return ((u + 0.055) / 1.055) ** 2.4


def _linear_to_srgb(u: float) -> float:
    u = _clampf(u)
    if u <= 0.0031308:
        return 12.92 * u
    return 1.055 * (u ** (1.0 / 2.4)) - 0.055


def pq_oetf(nits: float) -> float:
    """ST 2084 (PQ) OETF: absolute luminance (nits) -> signal (0..1)."""

    # ITU-R BT.2100 constants
    m1 = 2610.0 / 16384.0
    m2 = 2523.0 / 32.0
    c1 = 3424.0 / 4096.0
    c2 = 2413.0 / 128.0
    c3 = 2392.0 / 128.0

    # PQ is defined for 0..10000 nits
    l = _clampf(nits / 10000.0)
    lm1 = l ** m1
    num = c1 + c2 * lm1
    den = 1.0 + c3 * lm1
    return (num / den) ** m2


def pq_eotf(sig: float) -> float:
    """ST 2084 (PQ) EOTF: signal (0..1) -> absolute luminance (nits)."""

    m1 = 2610.0 / 16384.0
    m2 = 2523.0 / 32.0
    c1 = 3424.0 / 4096.0
    c2 = 2413.0 / 128.0
    c3 = 2392.0 / 128.0

    v = _clampf(sig) ** (1.0 / m2)
    num = max(v - c1, 0.0)
    den = c2 - c3 * v
    if den <= 0:
        return 10000.0
    l = (num / den) ** (1.0 / m1)
    return l * 10000.0


def ycbcr_to_rgb(y: int, cb: int, cr: int, *, matrix: str, limited: bool) -> Tuple[int, int, int]:
    """Convert 8-bit YCbCr to 8-bit RGB.

    This matches the math we're using in the ffmpeg patch (limited-range CCIR scaling).
    """

    cb_s = cb - 128
    cr_s = cr - 128

    if limited:
        y_f = (y - 16) * (255.0 / 219.0)
        cb_f = cb_s * (255.0 / 224.0)
        cr_f = cr_s * (255.0 / 224.0)
    else:
        y_f = float(y)
        cb_f = float(cb_s)
        cr_f = float(cr_s)

    if matrix == "bt601":
        r = y_f + 1.4020 * cr_f
        g = y_f - 0.344136 * cb_f - 0.714136 * cr_f
        b = y_f + 1.7720 * cb_f
    elif matrix == "bt2020":
        r = y_f + 1.4746 * cr_f
        g = y_f - 0.16455 * cb_f - 0.57135 * cr_f
        b = y_f + 1.8814 * cb_f
    else:  # bt709
        r = y_f + 1.5748 * cr_f
        g = y_f - 0.1873 * cb_f - 0.4681 * cr_f
        b = y_f + 1.8556 * cb_f

    return _clamp8(r), _clamp8(g), _clamp8(b)


def transparency_to_alpha(a_transparency: int, *, invert: bool) -> int:
    """Convert PGS transparency to alpha.

    PGS stores transparency (0 opaque .. 255 transparent).

    ffmpeg's stock RGBA expects alpha (0 transparent .. 255 opaque). Our patch historically had
    an inversion toggle; this helper reproduces both modes.

    - invert=True: treat PGS value as transparency and invert to alpha (recommended)
    - invert=False: treat PGS value as already alpha (useful for debugging broken streams)
    """

    if invert:
        return 255 - a_transparency
    return a_transparency


def iter_pg_packets(data: bytes) -> Iterator[Tuple[int, int, List[Tuple[int, bytes]]]]:
    """Yield (pts, dts, segments) for each PG packet."""

    i = 0
    n = len(data)
    while i + 2 <= n:
        if data[i : i + 2] != b"PG":
            j = data.find(b"PG", i + 1)
            if j == -1:
                return
            i = j
            continue
        if i + 10 > n:
            return
        pts = struct.unpack_from(">I", data, i + 2)[0]
        dts = struct.unpack_from(">I", data, i + 6)[0]
        i += 10
        segs: List[Tuple[int, bytes]] = []
        while i + 3 <= n:
            if data[i : i + 2] == b"PG":
                break
            seg_type = data[i]
            seg_len = struct.unpack_from(">H", data, i + 1)[0]
            i += 3
            if i + seg_len > n:
                return
            payload = data[i : i + seg_len]
            i += seg_len
            segs.append((seg_type, payload))
        yield pts, dts, segs


def parse_palette(payload: bytes) -> Optional[Palette]:
    if len(payload) < 2:
        return None
    palette_id = payload[0]
    version = payload[1]
    entries: Dict[int, PaletteEntry] = {}
    off = 2
    while off + 5 <= len(payload):
        idx = payload[off]
        y = payload[off + 1]
        cr = payload[off + 2]
        cb = payload[off + 3]
        a = payload[off + 4]
        entries[idx] = PaletteEntry(idx=idx, y=y, cb=cb, cr=cr, a_transparency=a)
        off += 5
    return Palette(palette_id=palette_id, version=version, entries=entries)


def parse_object(payload: bytes) -> Optional[Tuple[int, int, int, int, Optional[int], Optional[int], bytes]]:
    """Return (obj_id, obj_ver, seq_desc, total_len, w, h, data_fragment)."""

    if len(payload) < 7:
        return None
    obj_id = struct.unpack_from(">H", payload, 0)[0]
    obj_ver = payload[2]
    seq_desc = payload[3]
    total_len = int.from_bytes(payload[4:7], "big")

    off = 7
    w = h = None
    if seq_desc & 0x80:
        if off + 4 > len(payload):
            return None
        w = struct.unpack_from(">H", payload, off)[0]
        h = struct.unpack_from(">H", payload, off + 2)[0]
        off += 4

    return obj_id, obj_ver, seq_desc, total_len, w, h, payload[off:]


def parse_presentation(payload: bytes) -> Optional[Presentation]:
    # PCS (Presentation Composition Segment) includes a palette_update_flag byte.
    if len(payload) < 12:
        return None
    video_w = struct.unpack_from(">H", payload, 0)[0]
    video_h = struct.unpack_from(">H", payload, 2)[0]
    # payload[8] is palette_update_flag, payload[9] is palette_id
    palette_id = payload[9]
    obj_count = payload[10]
    off = 11
    objs: List[PresentedObject] = []
    for _ in range(obj_count):
        if off + 8 > len(payload):
            break
        obj_id = struct.unpack_from(">H", payload, off)[0]
        win_id = payload[off + 2]
        comp_flag = payload[off + 3]
        x = struct.unpack_from(">H", payload, off + 4)[0]
        y = struct.unpack_from(">H", payload, off + 6)[0]
        off += 8
        crop = None
        if comp_flag & 0x80:
            if off + 8 > len(payload):
                break
            crop = tuple(struct.unpack_from(">HHHH", payload, off))  # x,y,w,h
            off += 8
        objs.append(PresentedObject(obj_id=obj_id, win_id=win_id, comp_flag=comp_flag, x=x, y=y, crop=crop))
    return Presentation(video_w=video_w, video_h=video_h, palette_id=palette_id, objects=objs)


def decode_rle(width: int, height: int, rle: bytes) -> bytes:
    """Decode PGS RLE into an index bitmap (width*height).

    Matches FFmpeg's decode logic in libavcodec/pgssubdec.c.
    """

    out = bytearray(width * height)
    total = width * height

    pixel_count = 0
    line_count = 0
    p = 0

    while p < len(rle) and line_count < height:
        color = rle[p]
        p += 1
        run = 1

        if color == 0x00:
            if p >= len(rle):
                break
            flags = rle[p]
            p += 1

            run = flags & 0x3F
            if flags & 0x40:
                if p >= len(rle):
                    break
                run = (run << 8) + rle[p]
                p += 1

            # If 0x80 is set, an explicit color byte follows; otherwise color=0.
            if flags & 0x80:
                if p >= len(rle):
                    break
                color = rle[p]
                p += 1
            else:
                color = 0

        if run > 0:
            if pixel_count + run > total:
                run = total - pixel_count
            if run > 0:
                out[pixel_count : pixel_count + run] = bytes([color]) * run
                pixel_count += run
        else:
            # New line
            line_count += 1

    return bytes(out)


def write_pgm(path: Path, width: int, height: int, pixels: bytes) -> None:
    header = f"P5\n{width} {height}\n255\n".encode("ascii")
    path.write_bytes(header + pixels)


def write_ppm(path: Path, width: int, height: int, rgb: bytes) -> None:
    header = f"P6\n{width} {height}\n255\n".encode("ascii")
    path.write_bytes(header + rgb)


def write_png_rgb(path: Path, width: int, height: int, rgb: bytes) -> None:
    if Image is None:
        raise RuntimeError("Pillow not installed; cannot write PNG")
    img = Image.frombytes("RGB", (width, height), rgb)
    img.save(path)


def write_png_rgba(path: Path, width: int, height: int, rgba: bytes) -> None:
    if Image is None:
        raise RuntimeError("Pillow not installed; cannot write PNG")
    img = Image.frombytes("RGBA", (width, height), rgba)
    img.save(path)


def bitmap_histogram(index_bitmap: bytes) -> List[int]:
    hist = [0] * 256
    for v in index_bitmap:
        hist[v] += 1
    return hist


def top_indices(hist: List[int], *, skip_zero: bool, limit: int) -> List[Tuple[int, int]]:
    pairs = [(idx, cnt) for idx, cnt in enumerate(hist) if cnt]
    if skip_zero:
        pairs = [(idx, cnt) for idx, cnt in pairs if idx != 0]
    pairs.sort(key=lambda t: t[1], reverse=True)
    return pairs[:limit]


def bitmap_to_rgb(
    index_bitmap: bytes,
    width: int,
    height: int,
    palette: Palette,
    *,
    matrix: str,
    limited: bool,
    alpha_invert: bool,
) -> bytes:
    # Render onto black using alpha.
    out = bytearray(width * height * 3)
    for i, idx in enumerate(index_bitmap):
        e = palette.entries.get(idx)
        if e is None:
            r = g = b = 0
            a = 0
        else:
            r, g, b = ycbcr_to_rgb(e.y, e.cb, e.cr, matrix=matrix, limited=limited)
            a = transparency_to_alpha(e.a_transparency, invert=alpha_invert)
        # alpha blend on black
        r = (r * a) // 255
        g = (g * a) // 255
        b = (b * a) // 255
        out[i * 3 + 0] = r
        out[i * 3 + 1] = g
        out[i * 3 + 2] = b
    return bytes(out)


def bitmap_to_rgb_pq_code(
    index_bitmap: bytes,
    width: int,
    height: int,
    palette: Palette,
    *,
    matrix: str,
    limited: bool,
    alpha_invert: bool,
    pq_gain: float,
) -> bytes:
    """Return an RGB image where channels are PQ code values.

    Intended for visualizing the raw BT.2020 + ST2084 code values derived from
    the subtitle CLUT (UHD-BR: "into 8-bit BT.2020 ST 2084 Y'CbCr + alpha").
    PNG viewers will still treat this as SDR, so use pqpreview for a viewable
    approximation.
    """

    out = bytearray(width * height * 3)
    for i, idx in enumerate(index_bitmap):
        e = palette.entries.get(idx)
        if e is None:
            r = g = b = 0
            a = 0
        else:
            r, g, b = ycbcr_to_rgb(e.y, e.cb, e.cr, matrix=matrix, limited=limited)
            a = transparency_to_alpha(e.a_transparency, invert=alpha_invert)

        # r/g/b here are treated as PQ code values already.
        # Optional gain is applied in the decoded domain for experimentation.
        # For pqcode output we keep it as codes; gain is kept for parity/CLI.
        if pq_gain != 1.0:
            rn = pq_eotf(r / 255.0) * pq_gain
            gn = pq_eotf(g / 255.0) * pq_gain
            bn = pq_eotf(b / 255.0) * pq_gain
            r = _clamp8(pq_oetf(rn) * 255.0)
            g = _clamp8(pq_oetf(gn) * 255.0)
            b = _clamp8(pq_oetf(bn) * 255.0)

        # Visualize the common mistake: alpha blend in PQ code-value domain.
        af = a / 255.0
        out[i * 3 + 0] = _clamp8(r * af)
        out[i * 3 + 1] = _clamp8(g * af)
        out[i * 3 + 2] = _clamp8(b * af)
    return bytes(out)


def bitmap_to_rgb_pq_sdr_preview(
    index_bitmap: bytes,
    width: int,
    height: int,
    palette: Palette,
    *,
    matrix: str,
    limited: bool,
    alpha_invert: bool,
    pq_gain: float,
    sdr_white_nits: float,
) -> bytes:
    """Simulate an HDR PQ pipeline (BT.2020 + ST2084), then tonemap to SDR."""

    out = bytearray(width * height * 3)
    for i, idx in enumerate(index_bitmap):
        e = palette.entries.get(idx)
        if e is None:
            r = g = b = 0
            a = 0
        else:
            r, g, b = ycbcr_to_rgb(e.y, e.cb, e.cr, matrix=matrix, limited=limited)
            a = transparency_to_alpha(e.a_transparency, invert=alpha_invert)

        # Treat palette-converted RGB as PQ code values already.
        rn = pq_eotf(r / 255.0) * pq_gain
        gn = pq_eotf(g / 255.0) * pq_gain
        bn = pq_eotf(b / 255.0) * pq_gain

        # Apply alpha in linear-light (nits) onto black
        rn *= (a / 255.0)
        gn *= (a / 255.0)
        bn *= (a / 255.0)

        # Simple tonemap to SDR range (Reinhard-like): x/(x+W)
        r_sdr_lin = rn / (rn + sdr_white_nits)
        g_sdr_lin = gn / (gn + sdr_white_nits)
        b_sdr_lin = bn / (bn + sdr_white_nits)

        # Encode as sRGB for PNG preview
        r8 = _clamp8(_linear_to_srgb(r_sdr_lin) * 255.0)
        g8 = _clamp8(_linear_to_srgb(g_sdr_lin) * 255.0)
        b8 = _clamp8(_linear_to_srgb(b_sdr_lin) * 255.0)

        out[i * 3 + 0] = r8
        out[i * 3 + 1] = g8
        out[i * 3 + 2] = b8
    return bytes(out)


def bitmap_to_rgba(
    index_bitmap: bytes,
    width: int,
    height: int,
    palette: Palette,
    *,
    matrix: str,
    limited: bool,
    alpha_invert: bool,
) -> bytes:
    out = bytearray(width * height * 4)
    for i, idx in enumerate(index_bitmap):
        e = palette.entries.get(idx)
        if e is None:
            r = g = b = 0
            a = 0
        else:
            r, g, b = ycbcr_to_rgb(e.y, e.cb, e.cr, matrix=matrix, limited=limited)
            a = transparency_to_alpha(e.a_transparency, invert=alpha_invert)
        out[i * 4 + 0] = r
        out[i * 4 + 1] = g
        out[i * 4 + 2] = b
        out[i * 4 + 3] = a
    return bytes(out)


def _best_for_event(items: List[Tuple[int, object]], packet_index: int, *, forward_window: int = 8):
    """Return the best matching item for an event at packet_index.

    Prefer the most recent item with idx <= packet_index.
    If none exist (some SUP streams reference data that appears slightly later),
    fall back to the earliest item with idx > packet_index within forward_window.
    """

    best_before = None
    best_before_idx = -1
    for idx, item in items:
        if idx <= packet_index and idx >= best_before_idx:
            best_before = item
            best_before_idx = idx
    if best_before is not None:
        return best_before

    best_after = None
    best_after_idx = None
    for idx, item in items:
        if idx > packet_index and idx <= packet_index + forward_window:
            if best_after_idx is None or idx < best_after_idx:
                best_after = item
                best_after_idx = idx
    return best_after


def load_sup(path: Path) -> Tuple[Dict[int, List[Tuple[int, Palette]]], Dict[int, List[Tuple[int, ObjectBitmap]]], List[Event]]:
    data = path.read_bytes()

    palette_hist: Dict[int, List[Tuple[int, Palette]]] = {}
    object_hist: Dict[int, List[Tuple[int, ObjectBitmap]]] = {}
    events: List[Event] = []

    assemblies: Dict[int, _ObjectAssembly] = {}

    order = 0
    packet_index = 0

    for pts, dts, segs in iter_pg_packets(data):
        packet_index += 1
        for seg_type, payload in segs:
            if seg_type == PALETTE_SEGMENT:
                pal = parse_palette(payload)
                if pal:
                    palette_hist.setdefault(pal.palette_id, []).append((packet_index, pal))
            elif seg_type == OBJECT_SEGMENT:
                parsed = parse_object(payload)
                if parsed:
                    obj_id, obj_ver, seq_desc, total_len, w, h, frag = parsed
                    first = bool(seq_desc & 0x80)
                    last = bool(seq_desc & 0x40)

                    if first or obj_id not in assemblies:
                        assemblies[obj_id] = _ObjectAssembly(
                            obj_id=obj_id,
                            obj_ver=obj_ver,
                            total_len=total_len,
                            width=w,
                            height=h,
                            data=bytearray(),
                        )
                    asm = assemblies[obj_id]

                    # Update width/height if first fragment carries them
                    if w is not None and h is not None:
                        asm.width = w
                        asm.height = h
                    # Some streams may change total_len across versions; keep latest.
                    asm.total_len = total_len

                    asm.data.extend(frag)

                    # Finalize if last fragment or we have enough bytes.
                    if (last or len(asm.data) >= asm.total_len) and asm.width and asm.height:
                        rle = bytes(asm.data[: asm.total_len])
                        bm = decode_rle(asm.width, asm.height, rle)
                        object_hist.setdefault(obj_id, []).append(
                            (
                                packet_index,
                                ObjectBitmap(obj_id=obj_id, width=asm.width, height=asm.height, index_bitmap=bm),
                            )
                        )
                        # Reset assembly for potential next object update.
                        assemblies.pop(obj_id, None)
            elif seg_type == PRESENTATION_SEGMENT:
                pres = parse_presentation(payload)
                if pres:
                    events.append(Event(order=order, pts_90k=pts, packet_index=packet_index, presentation=pres))
                    order += 1

    return palette_hist, object_hist, events


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--sup", type=Path, required=True)
    ap.add_argument("--out", type=Path, default=Path("pgs_out"))
    ap.add_argument("--render", action="store_true", help="Write PGM/PPM images for events")
    ap.add_argument(
        "--png",
        action="store_true",
        help="Also write PNGs (requires Pillow): RGB preview and RGBA with correct alpha",
    )
    ap.add_argument(
        "--hdr",
        choices=["none", "pqcode", "pqpreview"],
        default="none",
        help="Optional HDR simulation output: pqcode=PQ code values, pqpreview=PQ then tonemap to SDR",
    )
    ap.add_argument(
        "--pq-gain",
        type=float,
        default=1.0,
        help="Optional gain applied in PQ-decoded nits domain (helps test visibility boosts)",
    )
    ap.add_argument("--sdr-white-nits", type=float, default=100.0, help="SDR reference white (nits) for pqpreview")
    ap.add_argument("--event", type=int, default=None, help="Only dump a single event index")
    ap.add_argument("--matrix", choices=["bt601", "bt709", "bt2020"], default="bt709")
    ap.add_argument("--range", choices=["limited", "full"], default="limited")
    ap.add_argument("--alpha-invert", choices=["1", "0"], default="1")
    ap.add_argument("--top", type=int, default=12, help="Top palette indices to print")
    args = ap.parse_args()

    palette_hist, object_hist, events = load_sup(args.sup)

    args.out.mkdir(parents=True, exist_ok=True)

    meta = {
        "sup": str(args.sup),
        "palettes": sorted(list(palette_hist.keys())),
        "objects": sorted(list(object_hist.keys())),
        "events": len(events),
    }
    (args.out / "meta.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")

    limited = args.range == "limited"
    alpha_invert = args.alpha_invert == "1"

    def dump_event(ev: Event) -> None:
        pres = ev.presentation
        pal = _best_for_event(palette_hist.get(pres.palette_id, []), ev.packet_index)
        print(
            f"event {ev.order}: pts={ev.pts_90k/90000:.3f}s packet={ev.packet_index} "
            f"palette_id={pres.palette_id} video={pres.video_w}x{pres.video_h} objects={len(pres.objects)}"
        )
        if not pal:
            print("  (missing palette)")

        for po in pres.objects:
            obj = _best_for_event(object_hist.get(po.obj_id, []), ev.packet_index)
            if not obj:
                print(f"  obj {po.obj_id}: (missing object)")
                continue
            hist = bitmap_histogram(obj.index_bitmap)
            top = top_indices(hist, skip_zero=True, limit=args.top)
            print(f"  obj {po.obj_id}: pos=({po.x},{po.y}) size={obj.width}x{obj.height} top_idx={top[:8]}")

            if pal:
                for idx, cnt in top[:8]:
                    e = pal.entries.get(idx)
                    if not e:
                        continue
                    rgb = ycbcr_to_rgb(e.y, e.cb, e.cr, matrix=args.matrix, limited=limited)
                    alpha = transparency_to_alpha(e.a_transparency, invert=alpha_invert)
                    print(f"    idx={idx} cnt={cnt} y={e.y} cb={e.cb} cr={e.cr} aT={e.a_transparency} -> rgb={rgb} alpha={alpha}")

            if args.render:
                stem = f"event_{ev.order:03d}_obj_{po.obj_id}_x{po.x}_y{po.y}_{obj.width}x{obj.height}"
                write_pgm(args.out / f"{stem}.idx.pgm", obj.width, obj.height, obj.index_bitmap)
                if pal:
                    rgb = bitmap_to_rgb(
                        obj.index_bitmap,
                        obj.width,
                        obj.height,
                        pal,
                        matrix=args.matrix,
                        limited=limited,
                        alpha_invert=alpha_invert,
                    )
                    write_ppm(args.out / f"{stem}.{args.matrix}.{args.range}.ppm", obj.width, obj.height, rgb)

                    if args.png and Image is not None:
                        write_png_rgb(
                            args.out / f"{stem}.{args.matrix}.{args.range}.rgb.png",
                            obj.width,
                            obj.height,
                            rgb,
                        )
                        rgba = bitmap_to_rgba(
                            obj.index_bitmap,
                            obj.width,
                            obj.height,
                            pal,
                            matrix=args.matrix,
                            limited=limited,
                            alpha_invert=alpha_invert,
                        )
                        write_png_rgba(
                            args.out / f"{stem}.{args.matrix}.{args.range}.rgba.png",
                            obj.width,
                            obj.height,
                            rgba,
                        )

                        if args.hdr != "none":
                            if args.hdr == "pqcode":
                                pq_rgb = bitmap_to_rgb_pq_code(
                                    obj.index_bitmap,
                                    obj.width,
                                    obj.height,
                                    pal,
                                    matrix=args.matrix,
                                    limited=limited,
                                    alpha_invert=alpha_invert,
                                    pq_gain=args.pq_gain,
                                )
                                write_png_rgb(
                                    args.out / f"{stem}.{args.matrix}.{args.range}.pqcode.png",
                                    obj.width,
                                    obj.height,
                                    pq_rgb,
                                )
                            elif args.hdr == "pqpreview":
                                pq_prev = bitmap_to_rgb_pq_sdr_preview(
                                    obj.index_bitmap,
                                    obj.width,
                                    obj.height,
                                    pal,
                                    matrix=args.matrix,
                                    limited=limited,
                                    alpha_invert=alpha_invert,
                                    pq_gain=args.pq_gain,
                                    sdr_white_nits=args.sdr_white_nits,
                                )
                                write_png_rgb(
                                    args.out / f"{stem}.{args.matrix}.{args.range}.pqpreview.png",
                                    obj.width,
                                    obj.height,
                                    pq_prev,
                                )

    if args.event is not None:
        if args.event < 0 or args.event >= len(events):
            raise SystemExit(f"event out of range (0..{len(events)-1})")
        dump_event(events[args.event])
    else:
        for ev in events:
            dump_event(ev)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
