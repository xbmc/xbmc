#!/usr/bin/env python3
"""Flag printf-style CLog::Log calls added by a change.

Reads a unified diff on stdin and emits one ::warning annotation per added
line matching CLog::Log(...%...), the check Jenkins posts as review comments.
Kodi's logging uses fmt-style {} placeholders; a % in a CLog::Log call is
nearly always a leftover printf format. Exits 0: this is advisory.
"""

import re
import sys

FILE_HEADER = re.compile(r"^\+\+\+ b/(?P<path>.+)$")
HUNK_HEADER = re.compile(r"^@@ -\d+(?:,\d+)? \+(?P<start>\d+)(?:,\d+)? @@")
PATTERN = re.compile(r"CLog::Log.*%")
MESSAGE = (
    "CLog::Log uses fmt-style {} placeholders; please replace the printf-style "
    "format (see https://fmt.dev/latest/syntax.html)"
)


def main() -> int:
    path = None
    line = 0
    hits = 0
    for raw in sys.stdin:
        raw = raw.rstrip("\n")
        header = FILE_HEADER.match(raw)
        if header:
            path = header["path"]
            continue
        hunk = HUNK_HEADER.match(raw)
        if hunk:
            line = int(hunk["start"])
            continue
        if path is None or raw.startswith("---"):
            continue
        if raw.startswith("+"):
            if PATTERN.search(raw[1:]):
                hits += 1
                print(f"::warning file={path},line={line},title=printf-style logging::{MESSAGE}")
            line += 1
        elif raw.startswith(" "):
            line += 1
        # removed lines ("-") do not advance the new-file line counter
    print(f"{hits} printf-style CLog::Log line(s) added")
    return 0


if __name__ == "__main__":
    sys.exit(main())
