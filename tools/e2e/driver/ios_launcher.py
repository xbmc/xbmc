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
import socket
import subprocess
import time
from pathlib import Path

from driver.launcher import DEFAULT_WEBSERVER_PORT, GUISETTINGS_TEMPLATE

APP_NAME = "Kodi"


class IOSKodiProcess:
    def __init__(
        self,
        app_path: str | Path,
        device: str = "booted",
        port: int = DEFAULT_WEBSERVER_PORT,
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
        self.port = port
        self.startup_timeout = startup_timeout
        self._pid: int | None = None
        self._profile_dir: Path | None = None
        self._data_container: Path | None = None

    def _simctl(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["xcrun", "simctl", *args], check=check, capture_output=True, text=True
        )

    @property
    def screenshot_dir(self) -> Path:
        assert self._profile_dir is not None, "start() was not called"
        return self._profile_dir / "screenshots"

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

    def _reset_userdata(self) -> None:
        shutil.rmtree(self._profile_dir / "userdata", ignore_errors=True)

    def _seed_settings(self) -> None:
        userdata_dir = self._profile_dir / "userdata"
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
        self._pid = self._launch()
        self._wait_for_port()

    def _wait_for_port(self) -> None:
        deadline = time.monotonic() + self.startup_timeout
        while time.monotonic() < deadline:
            if not self._is_running():
                raise RuntimeError(
                    f"Kodi (pid {self._pid}) exited early while waiting for the "
                    "webserver to start."
                )
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.settimeout(1.0)
                if sock.connect_ex(("127.0.0.1", self.port)) == 0:
                    return
            time.sleep(1.0)
        raise TimeoutError(
            f"Kodi webserver did not open port {self.port} within {self.startup_timeout}s."
        )

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi process that has already been asked to quit.

        Returns 0 if the process actually disappears within the deadline, 1 otherwise
        (after force-terminating it) - mirrors the other launchers' exit-code contract
        closely enough for assert_clean_shutdown to work unmodified.
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if not self._is_running():
                return 0
            time.sleep(0.5)
        self._terminate()
        return 1

    def read_log(self) -> str:
        # Searched across the whole data container rather than read from
        # _profile_dir/temp/kodi.log: special://logpath does not resolve the same way
        # special://home does on this platform, so the log can land outside the
        # profile directory that settings and screenshots use.
        matches = list(self._data_container.rglob("kodi.log"))
        if not matches:
            return ""
        return matches[0].read_text(errors="replace")

    def read_guisettings(self) -> str:
        """The container's guisettings.xml as Kodi last wrote it - see KodiProcess.

        Read from _profile_dir, not searched for like kodi.log above: settings live
        under special://home, which does resolve to the profile directory here.
        """
        assert self._profile_dir is not None, "start() was not called"
        settings_file = self._profile_dir / "userdata" / "guisettings.xml"
        if not settings_file.exists():
            return ""
        return settings_file.read_text(errors="replace")

    def kill_if_running(self) -> None:
        if self._is_running():
            self._terminate()
