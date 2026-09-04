"""Launches Kodi's iOS/tvOS build in a booted Simulator via `xcrun simctl`.

Generic over both: nothing here is iOS-specific (the bundle id is read from
Info.plist, and simctl mechanics don't differ by device family), so the tvOS E2E
job reuses this class unmodified.

Like Android (driver/android_launcher.py), iOS Kodi has no "-p"/"--portable" CLI flag:
xbmc/platform/darwin/ios/IOSEAGLView.mm builds a plain default-constructed CAppParams()
directly, and CAppParamParser (the only thing that understands "-p") is never invoked
on ios/tvos - only on the macOS desktop build (xbmc/platform/darwin/osx/
XBMCApplication.mm). So it always reads/writes a single fixed profile, and any per-run
configuration has to be pre-seeded onto disk rather than passed as CLI args, exactly as
on Android.

Unlike Android, that profile isn't behind an adb bridge: the Simulator is a plain macOS
process sharing the host filesystem and network stack directly (it is not a separate
VM). `xcrun simctl get_app_container <device> <bundle-id> data` resolves the app's
sandboxed container straight to a local path this launcher can read/write/glob like any
other pathlib.Path - no push/pull step needed - and 127.0.0.1 reaches the webserver
with no forwarding step either.

xbmc/platform/darwin/ios-common/DarwinEmbedUtils.mm's IsIosSandboxed() (used to pick
"Documents" vs "Library/Preferences" as the writable-home subfolder) keys off a
hardcoded real-device sandbox path prefix ("/private/var/containers/Bundle/") that a
Simulator app's bundle path never matches, so it always resolves to "Library/
Preferences" here, never "Documents" as it would on a signed device - see
_resolve_profile_dir below.

Because the Simulator shares the host kernel, the launched app is a real, signalable
host process - `simctl launch`'s own stdout reports its pid, so wait_for_exit() can
poll it directly (os.kill(pid, 0)) instead of needing Android's package-name-based
`pidof` proxy.
"""

from __future__ import annotations

import os
import plistlib
import shutil
import subprocess
from pathlib import Path

from driver.instance import free_port, wait_for_webserver, wait_until_gone
from driver.launcher import GUISETTINGS_TEMPLATE

APP_NAME = "Kodi"
# Where macOS writes crash reports for Simulator processes, which run as ordinary
# host processes: the per-user directory on current releases, the system one on
# older ones.
DIAGNOSTIC_REPORT_DIRS = (
    Path.home() / "Library" / "Logs" / "DiagnosticReports",
    Path("/Library/Logs/DiagnosticReports"),
)


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
    def screenshot_dir(self) -> Path:
        return self.profile_dir / "screenshots"

    def _is_running(self) -> bool:
        if self._pid is None:
            return False
        try:
            os.kill(self._pid, 0)
            return True
        except ProcessLookupError:
            return False

    def _install(self) -> None:
        self._simctl("install", self.device, str(self.app_path))

    def _resolve_profile_dir(self) -> None:
        result = self._simctl("get_app_container", self.device, self.bundle_id, "data")
        self._data_container = Path(result.stdout.strip())
        self._profile_dir = self._data_container / "Library" / "Preferences" / APP_NAME

    @property
    def profile_dir(self) -> Path:
        assert self._profile_dir is not None, "start() was not called"
        return self._profile_dir

    def _reset_userdata(self) -> None:
        shutil.rmtree(self.profile_dir / "userdata", ignore_errors=True)

    def _seed_settings(self) -> None:
        userdata_dir = self.profile_dir / "userdata"
        userdata_dir.mkdir(parents=True, exist_ok=True)
        self.screenshot_dir.mkdir(parents=True, exist_ok=True)
        (userdata_dir / "guisettings.xml").write_text(
            GUISETTINGS_TEMPLATE.format(port=self.port, screenshot_dir=self.screenshot_dir)
        )

    def _launch(self) -> int:
        # stdout is "<bundle-id>: <pid>" on success.
        result = self._simctl("launch", self.device, self.bundle_id)
        return int(result.stdout.strip().rsplit(":", 1)[-1].strip())

    def _terminate(self) -> None:
        self._simctl("terminate", self.device, self.bundle_id, check=False)

    def start(self) -> None:
        # Best-effort: clears any process still running from a previous test in the
        # same session before we wipe its profile out from under it.
        self._terminate()
        self._install()
        self._resolve_profile_dir()
        # Only userdata, so a previous run's artifacts survive: leaves
        # screenshots/logs from a previous run in place for CI to collect if this run
        # doesn't produce its own.
        self._reset_userdata()
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

        simctl exposes no exit code, so this reports 0 once the process is gone
        within the deadline and 1 otherwise (after force-terminating it). A crash
        on the way out therefore shows up through crash_report(), not here.
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
        new_reports = sorted(self._crash_reports() - self._reports_before)
        sections = []
        for report in new_reports:
            head = "\n".join(report.read_text(errors="replace").splitlines()[:40])
            sections.append(f"{report}:\n{head}")
        return "\n\n".join(sections)

    def read_log(self) -> str:
        # Searched across the whole data container rather than read from
        # _profile_dir/temp/kodi.log: special://logpath does not resolve the same way
        # special://home does on this platform, so the log can land outside the
        # profile directory that settings and screenshots use.
        if self._data_container is None:
            return ""
        matches = list(self._data_container.rglob("kodi.log"))
        if not matches:
            return ""
        return matches[0].read_text(errors="replace")

    def read_guisettings(self) -> str:
        """The container's guisettings.xml as Kodi last wrote it - see KodiProcess.

        Read from _profile_dir, not searched for like kodi.log above: settings live
        under special://home, which does resolve to the profile directory here.
        """
        settings_file = self.profile_dir / "userdata" / "guisettings.xml"
        if not settings_file.exists():
            return ""
        return settings_file.read_text(errors="replace")

    def kill_if_running(self) -> None:
        if self._is_running():
            self._terminate()
