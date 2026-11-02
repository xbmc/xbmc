#!/usr/bin/env python3
"""Print the backtraces of kodi-test crashes and gather their reports for upload.

A crash ends kodi-test before gtest writes its XML, so the job log otherwise
shows only the signal. The test step points Linux cores and Windows minidumps
at --dumps; macOS writes its own crash reports. Always exits 0: the test step
itself decides the job result.
"""

import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
import time

# ReportCrash writes the report a few seconds after the process died
MACOS_REPORT_WAIT = 60


def run(command: list[str]) -> str:
    result = subprocess.run(command, capture_output=True, text=True, errors="replace")
    return result.stdout + result.stderr


def linux_backtrace(binary: str, core: str) -> str:
    if not shutil.which("gdb"):
        run(["sudo", "apt-get", "install", "-y", "-qq", "gdb"])
    return run(["gdb", "-batch", "-ex", "thread apply all bt", binary, core])


def windows_backtrace(binary: str, dump: str) -> str:
    kits = os.path.join(os.environ.get("ProgramFiles(x86)", ""), "Windows Kits", "10", "Debuggers")
    cdb = shutil.which("cdb") or next(
        (path for arch in ("x64", "x86", "arm64")
         if os.path.isfile(path := os.path.join(kits, arch, "cdb.exe"))), None)
    if not cdb:
        return "cdb not found; the minidump is uploaded instead\n"
    symbols = os.path.dirname(os.path.abspath(binary))
    return run([cdb, "-z", dump, "-y", symbols, "-i", symbols, "-lines", "-c", ".ecxr;kn;~*kn;q"])


def macos_backtrace(report: str) -> str:
    with open(report, encoding="utf-8") as f:
        f.readline()  # the header is a JSON document of its own
        body = json.load(f)
    images = body.get("usedImages", [])
    exception = body.get("exception", {})
    lines = [f"{exception.get('type', '')} {exception.get('signal', '')} "
             f"{exception.get('subtype', '')}".strip()]
    faulting = body.get("faultingThread", 0)
    lines.append(f"Thread {faulting} crashed:")
    for number, frame in enumerate(body.get("threads", [])[faulting].get("frames", [])):
        image = images[frame["imageIndex"]].get("name", "?") if "imageIndex" in frame else "?"
        symbol = frame.get("symbol", hex(frame.get("imageOffset", 0)))
        source = (f" ({frame['sourceFile']}:{frame.get('sourceLine', 0)})"
                  if "sourceFile" in frame else "")
        lines.append(f"#{number:<3} {image:<24} {symbol}{source}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, help="path of the kodi-test executable")
    parser.add_argument("--dumps", required=True, help="folder the test step wrote dumps to")
    parser.add_argument("--results", required=True, help="glob of gtest XML files")
    parser.add_argument("--out", required=True, help="folder to gather reports in for upload")
    args = parser.parse_args()

    crashes: list[tuple[str, str]] = []  # (report to upload or "", backtrace)
    if sys.platform == "darwin":
        name = os.path.basename(args.binary)
        reports = os.path.expanduser(f"~/Library/Logs/DiagnosticReports/**/{name}*.ips")
        # Without results the run ended early, most likely in a crash
        deadline = time.monotonic() + (
            0 if glob.glob(args.results, recursive=True) else MACOS_REPORT_WAIT)
        while not (found := glob.glob(reports, recursive=True)) and time.monotonic() < deadline:
            time.sleep(2)
        crashes = [(report, macos_backtrace(report)) for report in sorted(found)]
    elif sys.platform == "win32":
        crashes = [(dump, windows_backtrace(args.binary, dump))
                   for dump in sorted(glob.glob(os.path.join(args.dumps, "*.dmp")))]
    else:
        # A core is as large as the process, so only its backtrace is kept
        crashes = [("", linux_backtrace(args.binary, core))
                   for core in sorted(glob.glob(os.path.join(args.dumps, "core.*")))]

    if not crashes:
        return 0

    os.makedirs(args.out, exist_ok=True)
    for number, (report, backtrace) in enumerate(crashes):
        print(f"::error title=kodi-test crashed::Backtrace {number} in the job log and the "
              "crash-reports artifact")
        print(f"::group::Crash {number} backtrace")
        print(backtrace, end="")
        print("::endgroup::")
        with open(os.path.join(args.out, f"backtrace-{number}.txt"), "w", encoding="utf-8") as f:
            f.write(backtrace)
        if report:
            shutil.copy(report, args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
