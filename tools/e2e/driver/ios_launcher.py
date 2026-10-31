"""Launches Kodi's iOS or tvOS build in a booted Simulator via `xcrun simctl`.

Like Android, iOS Kodi has no -p flag and one fixed profile, so settings are
seeded on disk before launch. The Simulator shares the host's filesystem,
network and processes: the app's data container is a local path, 127.0.0.1
reaches the webserver, and the pid `simctl launch` prints can be probed with
os.kill().

IsIosSandboxed() (xbmc/platform/darwin/ios-common/DarwinEmbedUtils.mm) never
matches a Simulator bundle path, so the profile is under Library/Preferences
rather than Documents as on a device.
"""

from __future__ import annotations

import os
import plistlib
import subprocess
import time
from pathlib import Path

from driver.instance import free_port, wait_for_webserver, wait_until_gone
from driver.launcher import GUISETTINGS_TEMPLATE

APP_NAME = "Kodi"
# Per-user on current macOS releases, system-wide on older ones.
DIAGNOSTIC_REPORT_DIRS = (
    Path.home() / "Library" / "Logs" / "DiagnosticReports",
    Path("/Library/Logs/DiagnosticReports"),
)
# ReportCrash writes the report after the crashed process is gone.
CRASH_REPORT_GRACE_SECONDS = 5.0


class IOSKodiProcess:
    def __init__(
        self,
        app_path: str | Path,
        device: str = "booted",
        port: int | None = None,
        startup_timeout: float = 120.0,
    ):
        self.app_path = Path(app_path).resolve()
        if not self.app_path.exists():
            raise FileNotFoundError(
                f"Kodi.app not found at {self.app_path}. Build it first "
                "(xcodebuild against a --with-platform=ios-simulator toolchain), or "
                "set the KODI_APP environment variable."
            )

        info_plist = self.app_path / "Info.plist"
        with info_plist.open("rb") as f:
            self.bundle_id = plistlib.load(f)["CFBundleIdentifier"]

        self.device = device
        self.port = port if port is not None else free_port()
        self.startup_timeout = startup_timeout
        self.exit_code: int | None = None
        self._pid: int | None = None
        self._reports_before: set[Path] = set()
        self._profile_dir: Path | None = None
        self._data_container: Path | None = None

    def _simctl(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["xcrun", "simctl", *args], check=check, capture_output=True, text=True
        )

    @property
    def _screenshot_dir(self) -> Path:
        return self.profile_dir / "screenshots"

    def pull_screenshots(self) -> list[Path]:
        return sorted(self._screenshot_dir.glob("*.png"))

    def _is_running(self) -> bool:
        if self._pid is None:
            return False
        try:
            os.kill(self._pid, 0)
            return True
        except ProcessLookupError:
            return False

    def _install(self) -> None:
        # Uninstalled first so each test starts from an empty data container.
        self._simctl("uninstall", self.device, self.bundle_id, check=False)
        self._simctl("install", self.device, str(self.app_path))

    def _resolve_profile_dir(self) -> None:
        result = self._simctl("get_app_container", self.device, self.bundle_id, "data")
        self._data_container = Path(result.stdout.strip())
        self._profile_dir = self._data_container / "Library" / "Preferences" / APP_NAME

    @property
    def profile_dir(self) -> Path:
        assert self._profile_dir is not None, "start() was not called"
        return self._profile_dir

    def _seed_settings(self) -> None:
        userdata_dir = self.profile_dir / "userdata"
        userdata_dir.mkdir(parents=True, exist_ok=True)
        self._screenshot_dir.mkdir(parents=True, exist_ok=True)
        (userdata_dir / "guisettings.xml").write_text(
            GUISETTINGS_TEMPLATE.format(port=self.port, screenshot_dir=self._screenshot_dir)
        )

    def _launch(self) -> int:
        # stdout is "<bundle-id>: <pid>" on success.
        result = self._simctl("launch", self.device, self.bundle_id)
        return int(result.stdout.strip().rsplit(":", 1)[-1].strip())

    def _terminate(self) -> None:
        self._simctl("terminate", self.device, self.bundle_id, check=False)

    def start(self) -> None:
        # A previous test's process may still be running.
        self._terminate()
        self._install()
        self._resolve_profile_dir()
        self._seed_settings()
        self._reports_before = self._crash_reports()
        self._pid = self._launch()
        self._wait_for_port()

    def _wait_for_port(self) -> None:
        wait_for_webserver(
            self.port,
            self.startup_timeout,
            still_running=self._is_running,
            diagnostics=lambda: "Log tail:\n" + "\n".join(self.read_log().splitlines()[-40:]),
        )

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi process that has already been asked to quit.

        simctl exposes no exit code: 0 means Kodi went away in time, 1 that it
        had to be terminated. Crashes show in crash_report().
        """
        if wait_until_gone(self._is_running, timeout):
            self.exit_code = 0
        else:
            self._terminate()
            self.exit_code = 1
        return self.exit_code

    def _crash_reports(self) -> set[Path]:
        reports: set[Path] = set()
        for directory in DIAGNOSTIC_REPORT_DIRS:
            for pattern in (f"{APP_NAME}-*.ips", f"{APP_NAME}_*.crash"):
                reports.update(directory.glob(pattern))
        return reports

    def crash_report(self) -> str:
        """Crash reports macOS wrote for the app since start(), or ""."""
        deadline = time.monotonic() + CRASH_REPORT_GRACE_SECONDS
        new_reports = sorted(self._crash_reports() - self._reports_before)
        while not new_reports and time.monotonic() < deadline:
            time.sleep(0.5)
            new_reports = sorted(self._crash_reports() - self._reports_before)
        sections = []
        for report in new_reports:
            head = "\n".join(report.read_text(errors="replace").splitlines()[:40])
            sections.append(f"{report}:\n{head}")
        return "\n\n".join(sections)

    def read_log(self) -> str:
        # Searched for: special://logpath does not resolve under the profile
        # directory on this platform.
        if self._data_container is None:
            return ""
        matches = list(self._data_container.rglob("kodi.log"))
        if not matches:
            return ""
        return matches[0].read_text(errors="replace")

    def read_guisettings(self) -> str:
        """The container's guisettings.xml as Kodi last wrote it - see KodiProcess."""
        settings_file = self.profile_dir / "userdata" / "guisettings.xml"
        if not settings_file.exists():
            return ""
        return settings_file.read_text(errors="replace")

    def kill_if_running(self) -> None:
        if self._is_running():
            self._terminate()
