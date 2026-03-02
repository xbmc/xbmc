#!/usr/bin/env python3
import argparse
import struct
from dataclasses import dataclass


def clamp8(x: float) -> int:
    if x < 0:
        return 0
    if x > 255:
        return 255
    return int(x + 0.5)


def ycbcr_to_rgb(y: int, cr: int, cb: int):
    # Matches the common BT.601-ish conversion used in Kodi's overlay utils.
    r = 1.164 * (y - 16) + 1.596 * (cr - 128)
    g = 1.164 * (y - 16) - 0.391 * (cb - 128) - 0.813 * (cr - 128)
    b = 1.164 * (y - 16) + 2.018 * (cb - 128)
    return clamp8(r), clamp8(g), clamp8(b)


@dataclass
class PaletteEntry:
    idx: int
    y: int
    cr: int
    cb: int
    a: int

    def rgba(self):
        r, g, b = ycbcr_to_rgb(self.y, self.cr, self.cb)
        return self.a, r, g, b


def parse_pds(data: bytes):
    # PDS layout:
    #   palette_id (1), palette_version (1), then N * (idx, Y, Cr, Cb, A)
    if len(data) < 2:
        return None, None, []
    palette_id = data[0]
    palette_version = data[1]
    payload = data[2:]

    entries = []
    if len(payload) % 5 != 0:
        payload = payload[: len(payload) - (len(payload) % 5)]
    for i in range(0, len(payload), 5):
        idx, y, cr, cb, a = payload[i : i + 5]
        entries.append(PaletteEntry(idx=idx, y=y, cr=cr, cb=cb, a=a))
    return palette_id, palette_version, entries


def main():
    ap = argparse.ArgumentParser(description="Dump PGS (SUP) palette definition segments (PDS).")
    ap.add_argument("sup", help="Path to .sup extracted from MKV/M2TS")
    ap.add_argument("--first", action="store_true", help="Stop after first PDS")
    ap.add_argument("--limit", type=int, default=16, help="Entries to print per PDS")
    args = ap.parse_args()

    with open(args.sup, "rb") as f:
        blob = f.read()

    # Blu-ray SUP/PGS files are a sequence of packets:
    #   'PG' (2) + PTS (4) + DTS (4) + seg_type (1) + seg_len (2) + seg_data (seg_len)
    # Many files have exactly one segment per packet (incl. separate END packets),
    # so we advance by the computed packet length and resync on errors.
    off = 0
    pds_count = 0
    while off + 13 <= len(blob):
        if blob[off : off + 2] != b"PG":
            nxt = blob.find(b"PG", off + 1)
            if nxt == -1:
                break
            off = nxt
            continue

        pts = struct.unpack_from(">I", blob, off + 2)[0]
        dts = struct.unpack_from(">I", blob, off + 6)[0]
        seg_type = blob[off + 10]
        seg_len = struct.unpack_from(">H", blob, off + 11)[0]

        seg_start = off + 13
        seg_end = seg_start + seg_len
        if seg_end > len(blob):
            # Corrupt length; resync.
            off += 2
            continue

        seg = blob[seg_start:seg_end]

        if seg_type == 0x14:  # PDS
            pds_count += 1
            palette_id, palette_version, entries = parse_pds(seg)
            print(
                f"PDS #{pds_count} pts={pts} dts={dts} palette_id={palette_id} palette_version={palette_version} entries={len(entries)}"
            )
            entries_sorted = sorted(entries, key=lambda e: e.idx)
            for e in entries_sorted[: args.limit]:
                a, r, g, b = e.rgba()
                print(
                    f"  idx={e.idx:3d} Y={e.y:3d} Cr={e.cr:3d} Cb={e.cb:3d} A={e.a:3d}  ->  A={a:3d} R={r:3d} G={g:3d} B={b:3d}"
                )
            if args.first:
                return

        off = seg_end

    if pds_count == 0:
        raise SystemExit("No PDS segments found (is this a PGS .sup?)")


if __name__ == "__main__":
    main()
