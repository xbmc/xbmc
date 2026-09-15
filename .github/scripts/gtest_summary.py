#!/usr/bin/env python3
"""Summarise gtest XML reports for a GitHub Actions job.

Writes a per-suite table to the step summary and one ::error annotation per
failed test, pointing at the file and line gtest recorded. Always exits 0: the
test step itself decides the job result, this only makes it readable.
"""

import argparse
import glob
import os
import re
import sys
import xml.etree.ElementTree as ET

LOCATION = re.compile(r"^(?P<file>[^\n:]+):(?P<line>\d+)")


def relative(path: str) -> str:
    workspace = os.environ.get("GITHUB_WORKSPACE")
    if workspace and os.path.isabs(path):
        try:
            return os.path.relpath(path, workspace)
        except ValueError:
            return path
    return path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("pattern", help="glob of gtest XML files")
    parser.add_argument("--title", default="Unit tests")
    args = parser.parse_args()

    files = sorted(glob.glob(args.pattern, recursive=True))
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    out = open(summary_path, "a", encoding="utf-8") if summary_path else sys.stdout

    if not files:
        print(f"### {args.title}\n\nNo gtest XML matched `{args.pattern}`.\n", file=out)
        print(f"::warning::no gtest XML matched {args.pattern}")
        return 0

    total = failed = skipped = 0
    rows = []
    for path in files:
        root = ET.parse(path).getroot()
        suites = [root] if root.tag == "testsuite" else root.findall("testsuite")
        for suite in suites:
            cases = suite.findall("testcase")
            suite_failed = 0
            for case in cases:
                total += 1
                if case.get("status") == "notrun" or case.find("skipped") is not None:
                    skipped += 1
                    continue
                failure = case.find("failure")
                if failure is None:
                    continue
                failed += 1
                suite_failed += 1
                name = f"{case.get('classname')}.{case.get('name')}"
                message = (failure.get("message") or failure.text or "").strip()
                lines = [l for l in message.splitlines() if l.strip()]
                match = LOCATION.match(message)
                location = ""
                if match:
                    location = f"file={relative(match['file'])},line={match['line']},"
                    lines = lines[1:]
                detail = " ".join(lines)[:500] if lines else "failed"
                print(f"::error {location}title={name}::{detail}")
            rows.append((suite.get("name"), len(cases), suite_failed))

    status = "passed" if failed == 0 else f"{failed} failed"
    print(f"### {args.title}: {total} tests, {status}, {skipped} skipped\n", file=out)
    failing = [r for r in rows if r[2]]
    if failing:
        print("| Suite | Tests | Failed |\n| --- | --- | --- |", file=out)
        for name, count, suite_failed in failing:
            print(f"| {name} | {count} | {suite_failed} |", file=out)
        print("", file=out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
