#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Write the ICC profile fixtures the FFmpegImage tests read.

    python3 tools/testdata/mkicc.py [outdir]

Writes to xbmc/guilib/test/testdata/ unless told otherwise. Needs Pillow and lcms2.

Each file is a flat 32x32 JPEG, RGB (200, 100, 100) or gray 128, untagged or tagged with a
profile that lcms2 builds from the primaries and curves below, so the files stay small and
nothing in them comes from a camera or a vendor. The profiles' creation date is fixed, so the
output is the same on every run with the same Pillow and lcms2.
"""

import ctypes
import ctypes.util
import struct
import sys
from pathlib import Path

from PIL import Image

SIZE = 32
COLOUR = (200, 100, 100)
GRAY = 128
CREATED = (2026, 1, 1, 0, 0, 0)  # year, month, day, hours, minutes, seconds

# CIE xy chromaticities: white point, then red, green and blue primaries
D65 = (0.3127, 0.3290)
DISPLAY_P3 = ((0.680, 0.320), (0.265, 0.690), (0.150, 0.060))
SRGB = ((0.64, 0.33), (0.30, 0.60), (0.15, 0.06))

lcms = ctypes.CDLL(ctypes.util.find_library("lcms2") or "liblcms2.so.2")


class XyY(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double), ("Y", ctypes.c_double)]


class Primaries(ctypes.Structure):
    _fields_ = [("red", XyY), ("green", XyY), ("blue", XyY)]


class XYZ(ctypes.Structure):
    _fields_ = [("X", ctypes.c_double), ("Y", ctypes.c_double), ("Z", ctypes.c_double)]


P = ctypes.c_void_p
for name, restype, argtypes in (
    ("cmsBuildParametricToneCurve", P, [P, ctypes.c_int, ctypes.POINTER(ctypes.c_double)]),
    ("cmsBuildTabulatedToneCurveFloat", P, [P, ctypes.c_uint32, ctypes.POINTER(ctypes.c_float)]),
    ("cmsCreateRGBProfile", P, [ctypes.POINTER(XyY), ctypes.POINTER(Primaries), P * 3]),
    ("cmsCreate_sRGBProfile", P, []),
    ("cmsCreateGrayProfile", P, [ctypes.POINTER(XyY), P]),
    ("cmsSetProfileVersion", None, [P, ctypes.c_double]),
    ("cmsReadTag", P, [P, ctypes.c_uint32]),
    ("cmsPipelineAlloc", P, [P, ctypes.c_uint32, ctypes.c_uint32]),
    ("cmsStageAllocToneCurves", P, [P, ctypes.c_uint32, P]),
    ("cmsStageAllocCLut16bit", P, [P, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32, P]),
    ("cmsPipelineInsertStage", ctypes.c_int, [P, ctypes.c_int, P]),
    ("cmsMLUalloc", P, [P, ctypes.c_uint32]),
    ("cmsMLUsetASCII", ctypes.c_int, [P, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]),
    ("cmsWriteTag", ctypes.c_int, [P, ctypes.c_uint32, P]),
    ("cmsSaveProfileToMem", ctypes.c_int, [P, P, ctypes.POINTER(ctypes.c_uint32)]),
):
    getattr(lcms, name).restype = restype
    getattr(lcms, name).argtypes = argtypes


def tag(signature: str) -> int:
    return struct.unpack(">I", signature.encode("ascii"))[0]


def srgb_curve():
    """The sRGB transfer function as an ICC parametric curve of type 4."""
    params = (ctypes.c_double * 5)(2.4, 1 / 1.055, 0.055 / 1.055, 1 / 12.92, 0.04045)
    return lcms.cmsBuildParametricToneCurve(None, 4, params)


def srgb_table(entries: int):
    """The sRGB transfer function sampled into a table, as v2 profiles carry it."""
    def eotf(v: float) -> float:
        return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4

    table = (ctypes.c_float * entries)(*[eotf(i / (entries - 1)) for i in range(entries)])
    return lcms.cmsBuildTabulatedToneCurveFloat(None, entries, table)


def rgb_profile(primaries, curve):
    white = XyY(*D65, 1.0)
    prims = Primaries(*(XyY(x, y, 1.0) for x, y in primaries))
    curves = (P * 3)(curve, curve, curve)
    return lcms.cmsCreateRGBProfile(ctypes.byref(white), ctypes.byref(prims), curves)


def gray_profile(gamma: float):
    white = XyY(*D65, 1.0)
    curve = lcms.cmsBuildParametricToneCurve(None, 1, (ctypes.c_double * 1)(gamma))
    return lcms.cmsCreateGrayProfile(ctypes.byref(white), curve)


def lut_profile():
    """sRGB's matrix and curves, plus a LUT that swaps red and blue, which lcms2 uses instead."""
    profile = lcms.cmsCreate_sRGBProfile()
    xyz = [
        ctypes.cast(lcms.cmsReadTag(profile, tag(sig)), ctypes.POINTER(XYZ)).contents
        for sig in ("rXYZ", "gXYZ", "bXYZ")
    ]
    # the LUT's input curves undo sRGB's transfer function, so its 2x2x2 table maps linear RGB to
    # XYZ, which interpolating the table does exactly; XYZ is encoded with 1.0 as 0x8000
    table = []
    for r in (0, 1):
        for g in (0, 1):
            for b in (0, 1):
                for axis in ("X", "Y", "Z"):
                    value = sum(w * getattr(c, axis) for w, c in zip((b, g, r), xyz))
                    table.append(round(value * 0x8000))
    curves = (P * 3)(srgb_curve(), srgb_curve(), srgb_curve())
    lut = lcms.cmsPipelineAlloc(None, 3, 3)
    for stage in (
        lcms.cmsStageAllocToneCurves(None, 3, curves),
        lcms.cmsStageAllocCLut16bit(None, 2, 3, 3, (ctypes.c_uint16 * len(table))(*table)),
        lcms.cmsStageAllocToneCurves(None, 3, None),
    ):
        lcms.cmsPipelineInsertStage(lut, 1, stage)  # at the end
    lcms.cmsWriteTag(profile, tag("A2B0"), lut)
    return profile


def save(profile, description: str) -> bytes:
    for signature, text in (("desc", description), ("cprt", "No copyright, use freely")):
        mlu = lcms.cmsMLUalloc(None, 1)
        lcms.cmsMLUsetASCII(mlu, b"en", b"US", text.encode("ascii"))
        lcms.cmsWriteTag(profile, tag(signature), mlu)
    size = ctypes.c_uint32(0)
    lcms.cmsSaveProfileToMem(profile, None, ctypes.byref(size))
    data = ctypes.create_string_buffer(size.value)
    lcms.cmsSaveProfileToMem(profile, data, ctypes.byref(size))
    icc = bytearray(data.raw[: size.value])
    # lcms2 stamps the time of saving into the header (dateTimeNumber at offset 24)
    icc[24:36] = struct.pack(">6H", *CREATED)
    return bytes(icc)


def fixtures():
    """Name, image and profile of each fixture."""
    rgb = Image.new("RGB", (SIZE, SIZE), COLOUR)
    gray = Image.new("L", (SIZE, SIZE), GRAY)
    srgb_v2 = rgb_profile(SRGB, srgb_table(1024))
    lcms.cmsSetProfileVersion(srgb_v2, 2.1)
    return (
        ("untagged", rgb, None),
        ("p3", rgb, save(rgb_profile(DISPLAY_P3, srgb_curve()), "Display P3 (test)")),
        ("srgb-v4", rgb, save(lcms.cmsCreate_sRGBProfile(), "sRGB v4 (test)")),
        ("srgb-v2", rgb, save(srgb_v2, "sRGB v2 table (test)")),
        ("srgb-lut", rgb, save(lut_profile(), "sRGB with a red and blue swapping LUT (test)")),
        ("gray-untagged", gray, None),
        ("gray", gray, save(gray_profile(2.2), "Gray gamma 2.2 (test)")),
    )


def main() -> None:
    outdir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("xbmc/guilib/test/testdata")
    outdir.mkdir(parents=True, exist_ok=True)
    for name, image, icc in fixtures():
        path = outdir / f"icc-{name}.jpg"
        extra = {"icc_profile": icc} if icc else {}
        image.save(path, quality=100, subsampling=0, **extra)
        print(f"wrote {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
