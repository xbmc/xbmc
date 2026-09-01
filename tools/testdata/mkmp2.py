#!/usr/bin/env python3
#
#  Copyright (C) 2026 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.
#
"""Write the MPEG Audio Layer II fixture the music tag tests read.

    python3 tools/testdata/mkmp2.py [outdir]

Writes to xbmc/music/tags/test/testdata/ unless told otherwise.

The file is an ID3v2.4 tag followed by silent MPEG-1 Layer II frames, each built field by field
from the tables below rather than encoded from audio, so it stays a few kilobytes and holds only
what the test it serves is about. Output is deterministic: regenerating is how a fixture stays
explainable after an edit.
"""

import struct
import sys
from pathlib import Path

# What the tag says. The test asserts these exact strings.
TITLE = "The Test Tone"
ARTIST = "The Test Band"
ALBUM = "Live At The Test Venue"

# MPEG-1 Layer II, 128 kbit/s, 44100 Hz, single channel. 144 * bitrate / samplerate, floored.
BITRATE = 128000
SAMPLERATE = 44100
FRAME_BYTES = 144 * BITRATE // SAMPLERATE
FRAME_COUNT = 8

# Header fields, MSB first across four bytes, per ISO/IEC 11172-3 section 2.4.1.3.
SYNC = 0x7FF  # 11 bits
VERSION_MPEG1 = 0b11
LAYER_II = 0b10
NO_CRC = 0b1
BITRATE_INDEX_128 = 0b1000  # Layer II, MPEG-1
SAMPLERATE_INDEX_44100 = 0b00
PADDING = 0b0
PRIVATE = 0b0
MODE_SINGLE_CHANNEL = 0b11
MODE_EXTENSION = 0b00
COPYRIGHT = 0b0
ORIGINAL = 0b0
EMPHASIS_NONE = 0b00


def frame_header() -> bytes:
    """The four header bytes, assembled a field at a time so each one is checkable."""
    bits = 0
    for value, width in (
        (SYNC, 11),
        (VERSION_MPEG1, 2),
        (LAYER_II, 2),
        (NO_CRC, 1),
        (BITRATE_INDEX_128, 4),
        (SAMPLERATE_INDEX_44100, 2),
        (PADDING, 1),
        (PRIVATE, 1),
        (MODE_SINGLE_CHANNEL, 2),
        (MODE_EXTENSION, 2),
        (COPYRIGHT, 1),
        (ORIGINAL, 1),
        (EMPHASIS_NONE, 2),
    ):
        bits = (bits << width) | value
    return struct.pack(">I", bits)


def audio() -> bytes:
    """Silent frames. The bytes after the header are zero, which decodes to silence."""
    header = frame_header()
    return (header + bytes(FRAME_BYTES - len(header))) * FRAME_COUNT


def syncsafe(value: int) -> bytes:
    """ID3v2 sizes carry seven bits per byte so they can never imitate a frame sync."""
    return bytes((value >> shift) & 0x7F for shift in (21, 14, 7, 0))


def text_frame(frame_id: str, text: str) -> bytes:
    """One ID3v2.4 text frame, UTF-8 encoded."""
    payload = b"\x03" + text.encode("utf-8")
    return frame_id.encode("ascii") + syncsafe(len(payload)) + b"\x00\x00" + payload


def id3v2() -> bytes:
    frames = (
        text_frame("TIT2", TITLE) + text_frame("TPE1", ARTIST) + text_frame("TALB", ALBUM)
    )
    return b"ID3" + b"\x04\x00" + b"\x00" + syncsafe(len(frames)) + frames


def main() -> None:
    outdir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("xbmc/music/tags/test/testdata")
    outdir.mkdir(parents=True, exist_ok=True)
    path = outdir / "tagged.mp2"
    path.write_bytes(id3v2() + audio())
    print(f"wrote {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
