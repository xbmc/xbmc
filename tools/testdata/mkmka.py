#!/usr/bin/env python3
#
#  Copyright (C) 2005-2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Write the Matroska fixtures the music tag tests read.

    python3 tools/testdata/mkmka.py [outdir]

Writes to xbmc/filesystem/test/data/audiobook/ unless told otherwise.

Each file is built element by element from the tables below rather than muxed from media, so it
holds only what the test it serves is about and stays around a kilobyte. Output is deterministic:
regenerating is how a fixture stays explainable after an edit.

Kodi reads these through two readers - TagLib's Matroska API, or FFmpeg's demuxer below the TagLib
version floor - so each fixture is worth checking against both.
"""

import struct
import sys
from pathlib import Path

TIMESCALE = 1000000  # ns per tick, so chapter times below are milliseconds


def vint(n, width=None):
    """EBML variable length integer."""
    if width is None:
        width = 1
        while n >= (1 << (7 * width)) - 1:
            width += 1
    return ((1 << (7 * width)) | n).to_bytes(width, "big")


def el(eid, payload):
    return bytes.fromhex(eid) + vint(len(payload)) + payload


def uint(eid, v):
    return el(eid, v.to_bytes(max(1, (v.bit_length() + 7) // 8), "big") if v else b"\x00")


def flt(eid, v):
    return el(eid, struct.pack(">d", v))


def txt(eid, s):
    return el(eid, s.encode("utf-8"))


def simple_tag(name, value):
    """A SimpleTag. The spec wants one per value, so repeats are how multiple values are written."""
    return el("67C8", txt("45A3", name) + txt("4487", value) + uint("4484", 1) + txt("447A", "und"))


def build(chapters, album_tags, chapter_tags, other_edition=None, other_edition_tags=(),
          omit_end_times=False):
    """chapters: (uid, start_ms, end_ms, display name). *_tags: (name, value) pairs."""
    duration = max((end for _uid, _start, end, _name in chapters), default=9000)

    header = el(
        "1A45DFA3",
        uint("4286", 1) + uint("42F7", 1) + uint("42F2", 4) + uint("42F3", 8)
        + txt("4282", "matroska") + uint("4287", 4) + uint("4285", 2),
    )
    info = el(
        "1549A966",
        uint("2AD7B1", TIMESCALE) + flt("4489", float(duration))
        + txt("4D80", "Kodi test generator") + txt("5741", "Kodi test generator")
        + txt("7BA9", "Live At The Test Venue"),
    )
    # One silent PCM track, enough for a demuxer to report a stream.
    tracks = el(
        "1654AE6B",
        el(
            "AE",
            uint("D7", 1) + uint("73C5", 1) + uint("83", 2) + txt("86", "A_PCM/INT/LIT")
            + txt("22B59C", "und")
            + el("E1", flt("B5", 48000.0) + uint("9F", 2) + uint("6264", 16)),
        ),
    )

    atoms = b"".join(
        el(
            "B6",
            uint("73C4", uid) + uint("91", start * TIMESCALE)
            # ChapterTimeEnd is optional in the spec and some taggers leave it out
            + (b"" if omit_end_times else uint("92", end * TIMESCALE))
            + uint("98", 0) + uint("4598", 1)
            + el("80", txt("85", name) + txt("437C", "eng")),
        )
        for uid, start, end, name in chapters
    )
    editions = (
        el("45B9", uint("45BC", 1) + uint("45DB", 1) + uint("45DD", 0) + atoms) if chapters else b""
    )
    if other_edition:
        # A second, non-default edition. Its chapters are not the ones a player follows.
        other_atoms = b"".join(
            el(
                "B6",
                uint("73C4", uid) + uint("91", start * TIMESCALE) + uint("92", end * TIMESCALE)
                + uint("98", 0) + uint("4598", 1)
                + el("80", txt("85", name) + txt("437C", "eng")),
            )
            for uid, start, end, name in other_edition
        )
        editions += el("45B9", uint("45BC", 2) + uint("45DB", 0) + uint("45DD", 0) + other_atoms)
    chapters_el = el("1043A770", editions) if editions else b""

    # TargetTypeValue 50 is the album, 30 a track - the distinction TagLib keeps and FFmpeg flattens.
    album = el(
        "7373",
        el("63C0", uint("68CA", 50) + txt("63CA", "ALBUM"))
        + b"".join(simple_tag(n, v) for n, v in album_tags),
    )
    tracks_tags = b"".join(
        el(
            "7373",
            el("63C0", uint("68CA", 30) + txt("63CA", "TRACK") + uint("63C4", uid))
            + b"".join(simple_tag(n, v) for n, v in chapter_tags(uid, index, name)),
        )
        for index, (uid, _start, _end, name) in enumerate(chapters, start=1)
        if chapter_tags(uid, index, name)
    )

    frame = b"\x00" * 192
    cluster = el("1F43B675", uint("E7", 0) + el("A3", b"\x81\x00\x00\x80" + frame))

    other_tags = b"".join(
        el(
            "7373",
            el("63C0", uint("68CA", 30) + txt("63CA", "TRACK") + uint("63C4", uid))
            + b"".join(simple_tag(n, v) for n, v in other_edition_tags),
        )
        for uid, _s, _e, _n in (other_edition or [])
    ) if other_edition_tags else b""

    tags = el("1254C367", album + tracks_tags + other_tags)

    return header + el("18538067", info + tracks + chapters_el + tags + cluster)


THREE = [
    (1001, 0, 3000, "Opening Number"),
    (1002, 3000, 6000, "Someone's Song"),
    (1003, 6000, 9000, "Encore"),
]

ALBUM = [("TITLE", "Live At The Test Venue"), ("ARTIST", "The Test Band"),
         ("DATE_RELEASED", "2026")]


def per_track(uid, index, name):
    return [("TITLE", name), ("ARTIST", "The Test Band"), ("PART_NUMBER", str(index))]


FIXTURES = {
    # An album as a well behaved tagger writes it. The apostrophe is deliberate: it reaches the
    # database through PrepareSQL.
    "chaptered.mka": lambda: build(THREE, ALBUM, per_track),
    # Chapters named only by ChapterDisplay. Without a fallback the tracks take the album title.
    "chapternames-only.mka": lambda: build(THREE, ALBUM, lambda *_: []),
    # A chapter's own TITLE must win over its display name.
    "precedence.mka": lambda: build(
        [(uid, s, e, "DISPLAY " + n) for uid, s, e, n in THREE],
        ALBUM,
        lambda uid, index, name: [("TITLE", "TAG " + name.removeprefix("DISPLAY "))],
    ),
    # A sub-second chapter one reader drops and the other counts, which is where the two lists
    # stop lining up.
    "microchapter.mka": lambda: build(
        [
            (1001, 0, 3000, "Opening Number"),
            (1002, 3000, 3400, "Stage Chatter"),
            (1003, 3400, 6400, "Someone's Song"),
            (1004, 6400, 9400, "Encore"),
        ],
        ALBUM,
        per_track,
    ),
    # PART_NUMBER disagreeing with file order: the tags number the tracks, not the position.
    "partnumber.mka": lambda: build(
        THREE, ALBUM, lambda uid, index, name: [("TITLE", name), ("PART_NUMBER", str(index + 4))]
    ),
    # One song in one file, the common case: album level tags and no chapters at all. This is what
    # CMusicInfoTagLoaderMatroska reads, the path that never goes through CAudioBookFileDirectory.
    "singlefile.mka": lambda: build([], ALBUM + [("COMPOSER", "Bill Evans")], lambda *_: []),
    # Chapters with no ChapterTimeEnd, which the spec allows. Each reader has to work the end out
    # for itself, and they have to reach the same answer.
    "noendtimes.mka": lambda: build(THREE, ALBUM, per_track, omit_end_times=True),
    # A second edition whose chapters carry their own tags. None of it describes a track this
    # file produces, so none of it may reach the selected edition's tracks or the album.
    "twoeditions.mka": lambda: build(
        THREE,
        ALBUM,
        per_track,
        other_edition=[(2001, 0, 5000, "Other edition track")],
        # COMPOSER, which the per-track tags do not set: a track level tag of the same name would
        # overwrite the contamination and hide it.
        other_edition_tags=[("TITLE", "FOREIGN TITLE"), ("COMPOSER", "Foreign Composer")],
    ),
    # Three composers as the spec asks for them. FFmpeg keeps only the last, TagLib all three.
    "repeated.mka": lambda: build(
        THREE,
        ALBUM + [("COMPOSER", "Bill Evans"), ("COMPOSER", "Miles Davis"),
                 ("COMPOSER", "Gil Evans")],
        per_track,
    ),
}


def main():
    default = Path(__file__).resolve().parents[2] / "xbmc/filesystem/test/data/audiobook"
    outdir = Path(sys.argv[1]) if len(sys.argv) > 1 else default
    outdir.mkdir(parents=True, exist_ok=True)
    for name, make in FIXTURES.items():
        data = make()
        (outdir / name).write_bytes(data)
        print(f"{name}: {len(data)} bytes")


if __name__ == "__main__":
    main()
