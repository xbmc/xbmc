"""Launches a Kodi binary with a disposable, pre-configured portable profile.

Kodi's "-p"/"--portable" flag makes it read/write its configuration from a
"portable_data" folder next to its resolved app root instead of the platform's normal
user config location (see xbmc/application/AppParams.h UserDirectoriesLocation::PORTABLE
and xbmc/settings/SettingsComponent.cpp). We use that to give each test run a fresh,
disposable profile without touching any real Kodi installation, and pre-seed
guisettings.xml so the JSON-RPC webserver is already enabled on first launch.

"App root" is not always just the binary's parent directory: on macOS Kodi builds as
a `Kodi.app` bundle, and its Darwin GetHomePath() (xbmc/Util.cpp) resolves the
executable at `Kodi.app/Contents/MacOS/Kodi` to `Kodi.app/Contents/Resources/Kodi/`,
not the build tree root. On Linux the binary already sits directly next to
system/userdata/etc.

Also exercised on Windows: kodi.exe supports -p/--portable the same way (win32's
WinMain.cpp parses argv through the same CAppParamParser as posix/main.cpp, unlike
Android/iOS), portable_data resolves next to the exe the same way, and Python's
subprocess.Popen.send_signal(signal.SIGTERM) is special-cased on Windows to call
TerminateProcess() rather than raise - no platform-specific code needed here.

An installed Kodi (from a .deb, say) cannot use -p: portable_data would have to live
under /usr/share/kodi. On Linux, xbmc/settings/SettingsComponent.cpp takes the
profile directory from KODI_DATA instead, so profile_dir switches to that: no -p,
KODI_DATA exported, and the same userdata/temp layout underneath. Windows and macOS
do not read KODI_DATA.
"""

from __future__ import annotations

import os
import signal
import subprocess
import sys
from pathlib import Path
from typing import TextIO

from driver.instance import free_port, wait_for_webserver


def _resolve_app_root(binary_path: Path) -> Path:
    """Finds the directory holding Kodi's system/userdata/portable_data tree.

    Usually the binary's own directory, except inside a macOS .app bundle
    (.../Kodi.app/Contents/MacOS/Kodi), where it's Contents/Resources/<AppName>/
    - binary_path.name is that AppName, since the bundle's executable is built with
    OUTPUT_NAME set to it (CMakeLists.txt).
    """
    for parent in binary_path.parents:
        if parent.suffix == ".app":
            return parent / "Contents" / "Resources" / binary_path.name
    return binary_path.parent


# Shared with driver/android_launcher.py, which has no local portable_data of its own
# to seed but needs the identical settings pushed onto the device before first launch.
#
# The empty <resolutions/> and <viewstates/> are required, not decorative:
# every ISubSettings implementation registered in CSettings::Load(const
# TiXmlNode*) (xbmc/settings/Settings.cpp) must return true for the whole
# load to succeed, and two don't tolerate a missing section the way the
# others do - CDisplaySettings::Load() hard-fails without <resolutions>, and
# CViewStateSettings::Load() *warns* "no <viewstates> tag found" but still
# returns false, which is easy to misread as tolerated since every other
# ISubSettings that warns on a missing section (e.g. CSkinSettings) returns
# true. Either one failing fails CSettings::Load() as a whole, and
# CSettings::Reset() then deletes this file and regenerates real Kodi
# defaults - including services.webserverauthentication back to true with an
# empty password, which NetworkServices::Start()'s auth guard treats as a
# reason to disable the webserver rather than start it.
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
        """Where Kodi writes kodi.log, which is not the same place on every platform.

        InitDirectoriesWin32() (xbmc/settings/SettingsComponent.cpp) points
        special://logpath at the profile root and special://temp at cache/, while every
        other platform points logpath at temp/. Reading the wrong path is silent: the
        file simply does not exist, read_log() returns "" and assert_clean_shutdown's
        FATAL check passes vacuously.
        """
        if sys.platform == "win32":
            return self.profile_dir / "kodi.log"
        return self.profile_dir / "temp" / "kodi.log"

    @property
    def process_output_path(self) -> Path:
        """Captured stdout/stderr of the Kodi process itself.

        Kept separate from log_path: if Kodi crashes before its own file logger is up
        (e.g. a dyld/library-load failure or an early abort()), kodi.log never gets
        created at all, and this is the only place the failure reason is visible.
        """
        return self.profile_dir / "temp" / "process-output.log"

    @property
    def screenshot_dir(self) -> Path:
        return self.profile_dir / "screenshots"

    def _seed_settings(self) -> None:
        userdata_dir = self.profile_dir / "userdata"
        userdata_dir.mkdir(parents=True, exist_ok=True)
        # debug.screenshotpath must be pre-set: if empty, CScreenShot::TakeScreenshot()
        # (xbmc/utils/Screenshot.cpp) opens an interactive folder-picker dialog instead
        # of writing the file, which would hang forever with no one there to click it.
        self.screenshot_dir.mkdir(parents=True, exist_ok=True)

        settings_file = userdata_dir / "guisettings.xml"
        # Safety net for callers that skip the conftest.py wipe (e.g. ad-hoc KodiProcess use).
        if not settings_file.exists():
            settings_file.write_text(
                GUISETTINGS_TEMPLATE.format(port=self.port, screenshot_dir=self.screenshot_dir)
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

        # Redirect straight to a file rather than subprocess.PIPE: nothing drains a
        # PIPE while we're polling for the port below, and a chatty process (--debug
        # logging included) can fill that pipe's OS buffer and deadlock.
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

        Falls back to SIGTERM then SIGKILL if it doesn't exit in time, so a hung
        process never leaks and hangs CI.
        """
        assert self.process is not None, "start() was not called"
        try:
            returncode = self.process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self.process.send_signal(signal.SIGTERM)
            try:
                returncode = self.process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                self.process.kill()
                returncode = self.process.wait(timeout=15)

        self._close_output_file()
        self.exit_code = returncode
        return returncode

    def crash_report(self) -> str:
        """Always "": a crash already shows in the exit code here (negative for a
        signal on POSIX, the exception code on Windows)."""
        return ""

    def read_log(self) -> str:
        if not self.log_path.exists():
            return ""
        return self.log_path.read_text(errors="replace")

    def read_guisettings(self) -> str:
        """The profile's guisettings.xml as Kodi last wrote it, or "" if absent.

        Kodi flushes settings to disk only on a clean shutdown, so this is meaningful
        only after assert_clean_shutdown. Reading a setting back over JSON-RPC instead
        proves only that it reached the in-memory CSettings.
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
