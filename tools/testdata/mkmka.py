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

# EBML element IDs, as https://www.matroska.org/technical/elements.html names them.
EBML = "1A45DFA3"
EBML_VERSION = "4286"
EBML_READ_VERSION = "42F7"
EBML_MAX_ID_LENGTH = "42F2"
EBML_MAX_SIZE_LENGTH = "42F3"
DOC_TYPE = "4282"
DOC_TYPE_VERSION = "4287"
DOC_TYPE_READ_VERSION = "4285"
SEGMENT = "18538067"
INFO = "1549A966"
TIMESTAMP_SCALE = "2AD7B1"
DURATION = "4489"
MUXING_APP = "4D80"
WRITING_APP = "5741"
SEGMENT_TITLE = "7BA9"
TRACKS = "1654AE6B"
TRACK_ENTRY = "AE"
TRACK_NUMBER = "D7"
TRACK_UID = "73C5"
TRACK_TYPE = "83"
CODEC_ID = "86"
TRACK_LANGUAGE = "22B59C"
AUDIO = "E1"
SAMPLING_FREQUENCY = "B5"
CHANNELS = "9F"
BIT_DEPTH = "6264"
CHAPTERS = "1043A770"
EDITION_ENTRY = "45B9"
EDITION_UID = "45BC"
EDITION_FLAG_DEFAULT = "45DB"
EDITION_FLAG_ORDERED = "45DD"
CHAPTER_ATOM = "B6"
CHAPTER_UID = "73C4"
CHAPTER_TIME_START = "91"
CHAPTER_TIME_END = "92"
CHAPTER_FLAG_HIDDEN = "98"
CHAPTER_FLAG_ENABLED = "4598"
CHAPTER_DISPLAY = "80"
CHAP_STRING = "85"
CHAP_LANGUAGE = "437C"
TAGS = "1254C367"
TAG = "7373"
TARGETS = "63C0"
TARGET_TYPE_VALUE = "68CA"
TARGET_TYPE = "63CA"
TAG_CHAPTER_UID = "63C4"
SIMPLE_TAG = "67C8"
TAG_NAME = "45A3"
TAG_STRING = "4487"
TAG_DEFAULT = "4484"
TAG_LANGUAGE = "447A"

# TargetTypeValue: the level a tag applies to, under the spec's names for them.
# https://www.matroska.org/technical/tagging.html
TARGET_ALBUM = 50
TARGET_TRACK = 30
CLUSTER = "1F43B675"
TIMESTAMP = "E7"
SIMPLE_BLOCK = "A3"


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
    return el(
        SIMPLE_TAG,
        txt(TAG_NAME, name) + txt(TAG_STRING, value) + uint(TAG_DEFAULT, 1)
        + txt(TAG_LANGUAGE, "und"),
    )


def build(chapters, album_tags, chapter_tags, other_edition=None, other_edition_tags=(),
          omit_end_times=False, segment_title="Live At The Test Venue",
          omit_chapter_uids=False, album_target_level=TARGET_ALBUM, omit_duration=False,
          tag_chapter_uid=None):
    """chapters: (uid, start_ms, end_ms, display name). *_tags: (name, value) pairs."""
    duration = max((end for _uid, _start, end, _name in chapters), default=9000)

    header = el(
        EBML,
        uint(EBML_VERSION, 1) + uint(EBML_READ_VERSION, 1) + uint(EBML_MAX_ID_LENGTH, 4)
        + uint(EBML_MAX_SIZE_LENGTH, 8) + txt(DOC_TYPE, "matroska")
        + uint(DOC_TYPE_VERSION, 4) + uint(DOC_TYPE_READ_VERSION, 2),
    )
    info = el(
        INFO,
        uint(TIMESTAMP_SCALE, TIMESCALE)
        # Duration is optional too, and a file written without it leaves a last chapter that
        # declares no end with nothing at all to say where it stops.
        + (b"" if omit_duration else flt(DURATION, float(duration)))
        + txt(MUXING_APP, "Kodi test generator") + txt(WRITING_APP, "Kodi test generator")
        # The Segment title names the file. Optional, and taggers that write only album
        # level tags leave it out.
        + (txt(SEGMENT_TITLE, segment_title) if segment_title else b""),
    )
    # One silent PCM track, enough for a demuxer to report a stream.
    tracks = el(
        TRACKS,
        el(
            TRACK_ENTRY,
            uint(TRACK_NUMBER, 1) + uint(TRACK_UID, 1) + uint(TRACK_TYPE, 2)
            + txt(CODEC_ID, "A_PCM/INT/LIT") + txt(TRACK_LANGUAGE, "und")
            + el(AUDIO, flt(SAMPLING_FREQUENCY, 48000.0) + uint(CHANNELS, 2) + uint(BIT_DEPTH, 16)),
        ),
    )

    atoms = b"".join(
        el(
            CHAPTER_ATOM,
            # ChapterUID is optional in the spec; older muxers and audiobook tools leave it out
            (b"" if omit_chapter_uids else uint(CHAPTER_UID, uid))
            + uint(CHAPTER_TIME_START, start * TIMESCALE)
            # ChapterTimeEnd is optional in the spec and some taggers leave it out
            + (b"" if omit_end_times else uint(CHAPTER_TIME_END, end * TIMESCALE))
            + uint(CHAPTER_FLAG_HIDDEN, 0) + uint(CHAPTER_FLAG_ENABLED, 1)
            + el(CHAPTER_DISPLAY, txt(CHAP_STRING, name) + txt(CHAP_LANGUAGE, "eng")),
        )
        for uid, start, end, name in chapters
    )
    editions = (
        el(
            EDITION_ENTRY,
            uint(EDITION_UID, 1) + uint(EDITION_FLAG_DEFAULT, 1) + uint(EDITION_FLAG_ORDERED, 0)
            + atoms,
        )
        if chapters
        else b""
    )
    if other_edition:
        # A second, non-default edition. Its chapters are not the ones a player follows.
        other_atoms = b"".join(
            el(
                CHAPTER_ATOM,
                uint(CHAPTER_UID, uid) + uint(CHAPTER_TIME_START, start * TIMESCALE)
                + uint(CHAPTER_TIME_END, end * TIMESCALE) + uint(CHAPTER_FLAG_HIDDEN, 0)
                + uint(CHAPTER_FLAG_ENABLED, 1)
                + el(CHAPTER_DISPLAY, txt(CHAP_STRING, name) + txt(CHAP_LANGUAGE, "eng")),
            )
            for uid, start, end, name in other_edition
        )
        editions += el(
            EDITION_ENTRY,
            uint(EDITION_UID, 2) + uint(EDITION_FLAG_DEFAULT, 0) + uint(EDITION_FLAG_ORDERED, 0)
            + other_atoms,
        )
    chapters_el = el(CHAPTERS, editions) if editions else b""

    # The album/track distinction TagLib keeps and FFmpeg flattens.
    # An empty Targets is what a tagger writes when it declares no level at all.
    album_targets = (
        el(TARGETS, uint(TARGET_TYPE_VALUE, album_target_level) + txt(TARGET_TYPE, "ALBUM"))
        if album_target_level
        else el(TARGETS, b"")
    )
    album = el(TAG, album_targets + b"".join(simple_tag(n, v) for n, v in album_tags))
    tracks_tags = b"".join(
        el(
            TAG,
            el(
                TARGETS,
                uint(TARGET_TYPE_VALUE, TARGET_TRACK) + txt(TARGET_TYPE, "TRACK")
                # Normally the chapter's own UID. Overridden to write a ChapterUID the file does
                # not contain, which is what a stale or carried-over tag looks like.
                + uint(TAG_CHAPTER_UID, tag_chapter_uid if tag_chapter_uid else uid),
            )
            + b"".join(simple_tag(n, v) for n, v in chapter_tags(uid, index, name)),
        )
        for index, (uid, _start, _end, name) in enumerate(chapters, start=1)
        if chapter_tags(uid, index, name)
    )

    frame = b"\x00" * 192
    cluster = el(CLUSTER, uint(TIMESTAMP, 0) + el(SIMPLE_BLOCK, b"\x81\x00\x00\x80" + frame))

    other_tags = b"".join(
        el(
            TAG,
            el(
                TARGETS,
                uint(TARGET_TYPE_VALUE, TARGET_TRACK) + txt(TARGET_TYPE, "TRACK")
                + uint(TAG_CHAPTER_UID, uid),
            )
            + b"".join(simple_tag(n, v) for n, v in other_edition_tags),
        )
        for uid, _s, _e, _n in (other_edition or [])
    ) if other_edition_tags else b""

    tags = el(TAGS, album + tracks_tags + other_tags)

    return header + el(SEGMENT, info + tracks + chapters_el + tags + cluster)


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
    "singlefile.mka": lambda: build(
        [], ALBUM + [("COMPOSER", "Bill Evans")], lambda *_: [], segment_title="So What"
    ),
    # The same song with no Segment title, which is what names a track. Nothing but the album
    # level tags says what this song is.
    "singlefile-notitle.mka": lambda: build(
        [], ALBUM + [("COMPOSER", "Bill Evans")], lambda *_: [], segment_title=None
    ),
    # Chapters with no ChapterTimeEnd, which the spec allows. Each reader has to work the end out
    # for itself, and they have to reach the same answer.
    "noendtimes.mka": lambda: build(THREE, ALBUM, per_track, omit_end_times=True),
    # The same, and no Segment Duration either - both are optional. Nothing in the file says where
    # the last chapter stops, and a chapter of unknown length is still the last track.
    "noduration.mka": lambda: build(THREE, ALBUM, per_track, omit_end_times=True,
                                    omit_duration=True),
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
    # Two editions sharing a ChapterUID, which the spec permits: an ordered cut reusing a chapter
    # of the transfer. A UID both carry names a chapter this file does play, so the tags naming it
    # belong to that chapter rather than to the edition left behind. The display names differ from
    # the tagged ones so that a track falling back to its display name is visible.
    "sharedchapteruid.mka": lambda: build(
        [(uid, s, e, "DISPLAY " + n) for uid, s, e, n in THREE],
        ALBUM,
        lambda uid, index, name: [("TITLE", "TAG " + name.removeprefix("DISPLAY "))],
        other_edition=[(1001, 0, 3000, "Shared chapter")],
    ),
    # One chapter, and a track level tag naming a ChapterUID the file does not contain - a stale
    # UID left by a tagger, or one carried over from the file this was cut from. It names no track
    # of this file, so it describes the file rather than the only chapter there is. The Segment
    # title differs from it so that which of the two reached the file's tags is visible.
    "straychapteruid.mka": lambda: build(
        [(1001, 0, 9000, "DISPLAY Only Track")],
        ALBUM,
        lambda *_: [("TITLE", "TAG Stray Title")],
        segment_title="So What",
        tag_chapter_uid=4242,
    ),
    # ChapterUID 1, which the spec allows and taggers that number from one produce. A file whose
    # first chapter is UID 1 must still have its tags reach that chapter rather than the album.
    "lowchapteruid.mka": lambda: build(
        [(1, 0, 3000, "DISPLAY Opening Number"), (2, 3000, 6000, "DISPLAY Someone's Song")],
        ALBUM,
        # A title the display name cannot supply, and a number the file order cannot: both are the
        # chapter's own tags, which is what a lookup by ChapterUID has to find.
        lambda uid, index, name: [
            ("TITLE", "TAG " + name.removeprefix("DISPLAY ")),
            ("PART_NUMBER", str(index + 4)),
        ],
    ),
    # One chapter too short to be a track alongside one real one. The album is not expanded, so
    # the single song loader reads it - and must take the track, not the artefact.
    "onetrackplusartefact.mka": lambda: build(
        [(1001, 0, 400, "Stage Chatter"), (1002, 400, 3400, "Someone's Song")],
        ALBUM,
        lambda uid, index, name: [("TITLE", name)],
    ),
    # Chapters and nothing else: no tags, no Segment title. FFmpeg still reports the mandatory
    # MuxingApp as metadata, so what makes a file an album has to be more than "has metadata".
    "untagged.mka": lambda: build(THREE, [], lambda *_: [], segment_title=None),
    # A second edition whose chapter starts after the last of the default one. FFmpeg nests every
    # edition into one list and keeps whatever starts later, so this is where the two readers
    # genuinely part company on how many chapters a file has.
    "lateredition.mka": lambda: build(
        THREE, ALBUM, per_track, other_edition=[(2001, 10000, 12000, "Other edition track")]
    ),
    # Chapters with no ChapterUID, which the spec allows. TagLib gives them one and reads them;
    # FFmpeg requires a nonzero UID and reports none at all.
    "nochapteruid.mka": lambda: build(THREE, ALBUM, per_track, omit_chapter_uids=True),
    # Album tags carrying no TargetTypeValue, which is out of spec and common: the file says what
    # album it is without saying so at album level.
    # ... and its Segment title differs from the TITLE those tags carry, so that the two are told
    # apart: a tag the file states outright beats the container's own name for itself.
    "nolevel.mka": lambda: build(
        THREE, ALBUM, per_track, album_target_level=None, segment_title="Container Named Venue"
    ),
    # Three composers as the spec asks for them. FFmpeg keeps only the last, TagLib all three.
    "repeated.mka": lambda: build(
        THREE,
        ALBUM + [("COMPOSER", "Bill Evans"), ("COMPOSER", "Miles Davis"),
                 ("COMPOSER", "Gil Evans"),
                 ("ARTIST SORT", "Band, The"), ("ARTIST SORT", "Other, The")],
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
