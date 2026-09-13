#!/usr/bin/env python3
"""Convert clang-tidy text output and cppcheck XML (version 2) to SARIF 2.1.0.

Used by the static-analysis workflow to upload results to GitHub code scanning,
and with --annotate to print one ::warning per finding for pull request runs,
which cannot upload SARIF. Paths are made relative to --root so GitHub can map
them onto the repository.
"""

import argparse
import json
import os
import re
import sys
import xml.etree.ElementTree as ET

CLANG_TIDY_LINE = re.compile(
    r"^(?P<file>[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?P<level>warning|error): (?P<msg>.*?) \[(?P<check>[\w\-.,]+)\]$"
)


def relpath(path: str, root: str) -> str:
    if os.path.isabs(path):
        try:
            return os.path.relpath(path, root)
        except ValueError:
            return path
    return path


def parse_clang_tidy(path: str, root: str):
    seen = set()
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            match = CLANG_TIDY_LINE.match(line.rstrip("\n"))
            if not match:
                continue
            key = (match["file"], match["line"], match["col"], match["check"], match["msg"])
            if key in seen:
                continue
            seen.add(key)
            yield {
                "tool": "clang-tidy",
                "rule": match["check"],
                "level": "error" if match["level"] == "error" else "warning",
                "message": match["msg"],
                "file": relpath(match["file"], root),
                "line": int(match["line"]),
                "column": int(match["col"]),
            }


def parse_cppcheck(path: str, root: str):
    tree = ET.parse(path)
    for error in tree.getroot().iter("error"):
        severity = error.get("severity", "style")
        level = "error" if severity == "error" else ("warning" if severity == "warning" else "note")
        location = error.find("location")
        if location is None:
            continue
        yield {
            "tool": "cppcheck",
            "rule": error.get("id", "cppcheck"),
            "level": level,
            "message": error.get("verbose") or error.get("msg") or "",
            "file": relpath(location.get("file", ""), root),
            "line": int(location.get("line", "1") or 1),
            "column": int(location.get("column", "1") or 1),
        }


def sarif(findings, tool_name: str, tool_version: str):
    rules = {}
    results = []
    for f in findings:
        rules.setdefault(f["rule"], {"id": f["rule"], "shortDescription": {"text": f["rule"]}})
        results.append(
            {
                "ruleId": f["rule"],
                "level": f["level"],
                "message": {"text": f["message"]},
                "locations": [
                    {
                        "physicalLocation": {
                            "artifactLocation": {"uri": f["file"], "uriBaseId": "%SRCROOT%"},
                            "region": {"startLine": max(f["line"], 1), "startColumn": max(f["column"], 1)},
                        }
                    }
                ],
            }
        )
    return {
        "$schema": "https://json.schemastore.org/sarif-2.1.0.json",
        "version": "2.1.0",
        "runs": [
            {
                "tool": {"driver": {"name": tool_name, "version": tool_version, "rules": list(rules.values())}},
                "results": results,
            }
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--clang-tidy", help="clang-tidy text output")
    parser.add_argument("--cppcheck", help="cppcheck --xml-version=2 output")
    parser.add_argument("--root", default=os.environ.get("GITHUB_WORKSPACE", os.getcwd()))
    parser.add_argument("--sarif", help="write SARIF here")
    parser.add_argument("--tool-version", default="")
    parser.add_argument("--annotate", action="store_true", help="print ::warning annotations")
    parser.add_argument("--only-under", default="xbmc/", help="drop findings outside this path prefix")
    args = parser.parse_args()

    findings = []
    if args.clang_tidy and os.path.exists(args.clang_tidy):
        findings += list(parse_clang_tidy(args.clang_tidy, args.root))
    if args.cppcheck and os.path.exists(args.cppcheck):
        findings += list(parse_cppcheck(args.cppcheck, args.root))
    if args.only_under:
        findings = [f for f in findings if f["file"].startswith(args.only_under)]

    tool_name = "clang-tidy" if args.clang_tidy and not args.cppcheck else ("cppcheck" if args.cppcheck and not args.clang_tidy else "static-analysis")
    if args.sarif:
        with open(args.sarif, "w", encoding="utf-8") as out:
            json.dump(sarif(findings, tool_name, args.tool_version), out, indent=1)

    if args.annotate:
        for f in findings:
            kind = "error" if f["level"] == "error" else "warning"
            print(f"::{kind} file={f['file']},line={f['line']},col={f['column']},title={f['tool']} {f['rule']}::{f['message']}")

    by_rule = {}
    for f in findings:
        by_rule[f["rule"]] = by_rule.get(f["rule"], 0) + 1
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    out = open(summary_path, "a", encoding="utf-8") if summary_path else sys.stdout
    print(f"### {tool_name}: {len(findings)} finding(s)\n", file=out)
    if by_rule:
        print("| Check | Count |\n| --- | --- |", file=out)
        for rule, count in sorted(by_rule.items(), key=lambda kv: -kv[1]):
            print(f"| {rule} | {count} |", file=out)
        print("", file=out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
