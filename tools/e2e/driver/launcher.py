"""Launches a desktop Kodi binary with a disposable, pre-seeded profile.

The profile is "portable_data" next to the app root (-p), on Linux, Windows
and macOS alike. An installed Linux Kodi cannot use -p, since its app root is
not writable, so the profile is the directory KODI_DATA points at instead.
"""

import os
import signal
import subprocess
import sys
from pathlib import Path
from typing import TextIO

from driver.instance import free_port, wait_for_webserver


def _resolve_app_root(binary_path: Path) -> Path:
    """The directory holding Kodi's system/ and portable_data/ trees.

    The binary's own directory, except in a macOS bundle, where GetHomePath()
    (xbmc/Util.cpp) resolves Kodi.app/Contents/MacOS/<AppName> to
    Kodi.app/Contents/Resources/<AppName>.
    """
    for parent in binary_path.parents:
        if parent.suffix == ".app":
            return parent / "Contents" / "Resources" / binary_path.name
    return binary_path.parent


# Also seeded by the Android and Simulator launchers. <resolutions/> and
# <viewstates/> must be present: CDisplaySettings::Load() and
# CViewStateSettings::Load() fail without them, which fails CSettings::Load() as
# a whole and resets every setting to its default, the webserver to off.
GUISETTINGS_TEMPLATE = """<settings>
  <setting id="services.webserver">true</setting>
  <setting id="services.webserverport">{port}</setting>
  <setting id="services.webserverauthentication">false</setting>
  <setting id="debug.screenshotpath">{screenshot_dir}</setting>
  <!-- AUTO_UPDATES_NEVER: a repository fetch still running at quit time holds
       shutdown open past the launchers' exit timeout. -->
  <setting id="general.addonupdates">2</setting>
  <resolutions/>
  <viewstates/>
</settings>
"""


class KodiProcess:
    def __init__(
        self,
        binary_path: str | Path,
        port: int | None = None,
        startup_timeout: float = 120.0,
        profile_dir: str | Path | None = None,
    ):
        self.binary_path = Path(binary_path).resolve()
        if not self.binary_path.exists():
            raise FileNotFoundError(
                f"Kodi binary not found at {self.binary_path}. Build Kodi first, or "
                "set the KODI_BINARY environment variable."
            )

        self.port = port if port is not None else free_port()
        self.startup_timeout = startup_timeout
        self.process: subprocess.Popen | None = None
        self.exit_code: int | None = None
        self._app_root = _resolve_app_root(self.binary_path)
        self._profile_dir = Path(profile_dir).resolve() if profile_dir else None
        self._output_file: TextIO | None = None

    @property
    def profile_dir(self) -> Path:
        """portable_data next to the app root, or the directory given as profile_dir."""
        if self._profile_dir is not None:
            return self._profile_dir
        return self._app_root / "portable_data"

    @property
    def log_path(self) -> Path:
        """kodi.log: the profile root on Windows (InitDirectoriesWin32() in
        xbmc/settings/SettingsComponent.cpp), temp/ elsewhere. A wrong path fails
        silently, read_log() returning "" and the FATAL check passing."""
        if sys.platform == "win32":
            return self.profile_dir / "kodi.log"
        return self.profile_dir / "temp" / "kodi.log"

    @property
    def process_output_path(self) -> Path:
        """Kodi's stdout and stderr, the only trace of a failure before kodi.log
        is opened, such as a library that does not load."""
        return self.profile_dir / "temp" / "process-output.log"

    @property
    def _screenshot_dir(self) -> Path:
        return self.profile_dir / "screenshots"

    def pull_screenshots(self) -> list[Path]:
        return sorted(self._screenshot_dir.glob("*.png"))

    def _seed_settings(self) -> None:
        userdata_dir = self.profile_dir / "userdata"
        userdata_dir.mkdir(parents=True, exist_ok=True)
        # With an empty debug.screenshotpath, CScreenShot::TakeScreenshot() opens a
        # folder picker instead of writing the file.
        self._screenshot_dir.mkdir(parents=True, exist_ok=True)

        settings_file = userdata_dir / "guisettings.xml"
        # conftest.py wipes the profile before each test; an existing one is kept.
        if not settings_file.exists():
            settings_file.write_text(
                GUISETTINGS_TEMPLATE.format(port=self.port, screenshot_dir=self._screenshot_dir)
            )

    def start(self, extra_args: list[str] | None = None) -> None:
        self._seed_settings()

        args = [str(self.binary_path), "--debug"]
        env = None
        if self._profile_dir is None:
            args.append("-p")
        else:
            env = {**os.environ, "KODI_DATA": str(self._profile_dir)}
        if extra_args:
            args.extend(extra_args)

        # A file, not a PIPE: nothing drains a PIPE while start() polls, and
        # --debug output would fill it and block Kodi.
        self.process_output_path.parent.mkdir(parents=True, exist_ok=True)
        self._output_file = self.process_output_path.open("w")
        self.process = subprocess.Popen(
            args,
            stdout=self._output_file,
            stderr=subprocess.STDOUT,
            env=env,
        )
        self._wait_for_port()

    def _read_process_output_tail(self, lines: int = 40) -> str:
        if not self.process_output_path.exists():
            return "(no process output captured)"
        return "\n".join(self.process_output_path.read_text(errors="replace").splitlines()[-lines:])

    def _wait_for_port(self) -> None:
        process = self.process
        assert process is not None, "start() was not called"
        wait_for_webserver(
            self.port,
            self.startup_timeout,
            still_running=lambda: process.poll() is None,
            diagnostics=lambda: (
                f"Exit code: {process.returncode}. Captured output tail:\n"
                f"{self._read_process_output_tail()}"
            ),
        )

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi process that has already been asked to quit.

        Past the deadline it gets SIGTERM, then SIGKILL, and the result is 143 or
        137: Kodi exits 0 on SIGTERM, which would read as a clean shutdown.
        """
        assert self.process is not None, "start() was not called"
        try:
            returncode = self.process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self.process.send_signal(signal.SIGTERM)
            returncode = 128 + signal.SIGTERM
            try:
                self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=15)
                returncode = 128 + 9  # SIGKILL; signal.SIGKILL is missing on Windows

        self._close_output_file()
        self.exit_code = returncode
        return returncode

    def crash_report(self) -> str:
        """Always "": the exit code already reports a crash here."""
        return ""

    def read_log(self) -> str:
        if not self.log_path.exists():
            return ""
        return self.log_path.read_text(errors="replace")

    def read_guisettings(self) -> str:
        """The profile's guisettings.xml as Kodi last wrote it, or "" if absent.

        Kodi saves settings on a clean shutdown, so read this after
        assert_clean_shutdown.
        """
        settings_file = self.profile_dir / "userdata" / "guisettings.xml"
        if not settings_file.exists():
            return ""
        return settings_file.read_text(errors="replace")

    def kill_if_running(self) -> None:
        if self.process is not None and self.process.poll() is None:
            self.process.kill()
            self.process.wait(timeout=15)
        self._close_output_file()

    def _close_output_file(self) -> None:
        if self._output_file is not None:
            self._output_file.close()
            self._output_file = None
