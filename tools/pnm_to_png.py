#!/usr/bin/env python3
"""Convert Netpbm PPM/PGM images to PNG.

VS Code doesn't preview .ppm/.pgm by default. This script converts them to .png.

Usage:
  ./tools/pnm_to_png.py --in pgs_out --out pgs_out_png

Requires: Pillow
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="in_dir", type=Path, required=True)
    ap.add_argument("--out", dest="out_dir", type=Path, required=True)
    ap.add_argument("--glob", dest="glob", default="*.pp[mg]", help="Glob to match input files")
    ap.add_argument("--limit", type=int, default=0, help="0 = no limit")
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)

    files = sorted(args.in_dir.glob(args.glob))
    if args.limit and args.limit > 0:
        files = files[: args.limit]

    converted = 0
    for src in files:
        dst = args.out_dir / (src.stem + ".png")
        # Pillow auto-detects PPM/PGM.
        img = Image.open(src)
        img.save(dst)
        converted += 1

    print(f"Converted {converted} file(s) -> {args.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
