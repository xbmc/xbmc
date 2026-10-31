"""Launches Kodi's Android build on an adb-connected emulator.

Android Kodi has no -p flag and one fixed profile on external storage. On API 30
only root reaches it from adb, and a file root creates there gets an SELinux
label the app cannot read ("unable to load settings"). So this launcher creates
nothing: Kodi is launched once to build its own profile, and seeding rewrites
the contents of files Kodi made.

Pre-granting MANAGE_EXTERNAL_STORAGE skips the consent screens Splash shows on
first launch (tools/android/packaging/xbmc/src/Splash.java.in).
"""

import shlex
import shutil
import subprocess
import tempfile
import time
from pathlib import Path

from driver.instance import free_port, wait_for_webserver, wait_until_gone
from driver.launcher import GUISETTINGS_TEMPLATE

PACKAGE = "org.xbmc.kodi"
# Fixed because it is baked into the seeded guisettings.xml; the host side is
# forwarded per instance.
DEVICE_WEBSERVER_PORT = 18080
SPLASH_ACTIVITY = f"{PACKAGE}/{PACKAGE}.Splash"
# Kodi logs the profile path it resolved as "special://home/ is mapped to".
DATA_DIR = f"/storage/emulated/0/Android/data/{PACKAGE}/files/.kodi"
GUISETTINGS = f"{DATA_DIR}/userdata/guisettings.xml"
# special://temp/, which Kodi creates itself.
SCREENSHOT_DIR = f"{DATA_DIR}/temp"
PROFILE_SETTLE_SECONDS = 5.0


class AndroidKodiProcess:
    def __init__(
        self,
        apk_path: str | Path,
        serial: str | None = None,
        port: int | None = None,
        # First launch also unpacks the APK's assets into internal storage.
        startup_timeout: float = 180.0,
    ):
        self.apk_path = Path(apk_path).resolve()
        if not self.apk_path.exists():
            raise FileNotFoundError(
                f"Kodi APK not found at {self.apk_path}. Build it first (`make apk`), "
                "or set the KODI_APK environment variable."
            )

        self.port = port if port is not None else free_port()
        self.startup_timeout = startup_timeout
        self.exit_code: int | None = None
        self._serial = serial or ""
        self._adb_prefix = ["adb"] + (["-s", serial] if serial else [])
        self._local_dir = Path(tempfile.mkdtemp(prefix="kodi-e2e-android-"))

    def _adb(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [*self._adb_prefix, *args], check=check, capture_output=True, text=True
        )

    def _shell(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return self._adb("shell", *args, check=check)

    def _read_remote_file(self, remote_path: str) -> bytes | None:
        """Reads a device file as raw bytes; `exec-out`, unlike `shell`, does not
        pass them through a pty."""
        result = subprocess.run(
            [*self._adb_prefix, "exec-out", "cat", remote_path], capture_output=True
        )
        return result.stdout if result.returncode == 0 else None

    def _overwrite_remote_file(self, remote_path: str, content: str) -> None:
        """Rewrites an existing device file in place, keeping the SELinux label
        Kodi gave it; `adb push` to remote_path would create a new file."""
        staging_path = f"/data/local/tmp/{Path(remote_path).name}"
        with tempfile.NamedTemporaryFile("w", delete=False) as f:
            f.write(content)
            local_path = f.name
        try:
            self._adb("push", local_path, staging_path)
            # Quoted so the device shell gets the redirect.
            self._shell("sh", "-c", shlex.quote(f"cat {staging_path} > {remote_path}"))
        finally:
            Path(local_path).unlink(missing_ok=True)
            self._shell("rm", "-f", staging_path, check=False)

        written = self._read_remote_file(remote_path)
        if written is None or written.decode() != content:
            raise RuntimeError(
                f"{remote_path} did not verify after writing - "
                f"got {written!r}, expected {content!r}"
            )

    def pull_screenshots(self) -> list[Path]:
        """Copies the device's screenshots to a local directory and returns them.

        Only the PNGs: SCREENSHOT_DIR also holds kodi.log.
        """
        local = self._local_dir / "screenshots"
        local.mkdir(parents=True, exist_ok=True)
        listing = self._shell(
            "sh", "-c", shlex.quote(f"ls -1 {SCREENSHOT_DIR}/*.png"), check=False
        )
        pulled = []
        if listing.returncode == 0:
            for remote in listing.stdout.split():
                data = self._read_remote_file(remote)
                if data is not None:
                    path = local / Path(remote).name
                    path.write_bytes(data)
                    pulled.append(path)
        return sorted(pulled)

    def _is_running(self) -> bool:
        result = self._shell("pidof", PACKAGE, check=False)
        return result.returncode == 0 and bool(result.stdout.strip())

    def _logcat_tail(self, lines: int = 60) -> str:
        result = self._adb("logcat", "-d", "-s", "Kodi:V", "*:F", check=False)
        if result.returncode != 0:
            return "(no logcat captured)"
        return "\n".join(result.stdout.splitlines()[-lines:])

    def _restart_adbd_as_root(self) -> None:
        """Root is available on google_apis emulator images, not on Play Store
        images or retail devices."""
        self._adb("root", check=False)
        self._adb("wait-for-device", check=False)

    def _wait_for_profile_settled(self, deadline: float) -> None:
        """Waits until nothing under userdata has changed for PROFILE_SETTLE_SECONDS.

        Kodi keeps creating the profile, the databases in particular, after
        guisettings.xml appears. Stopping it in between leaves a database without
        its schema, and the next launch fails on "no such table: version".
        """
        previous_listing = None
        settled_at = None
        while time.monotonic() < deadline:
            listing = self._shell(
                "sh", "-c", shlex.quote(f"ls -lR {DATA_DIR}/userdata"), check=False
            ).stdout
            if listing != previous_listing:
                previous_listing = listing
                settled_at = None
            elif settled_at is None:
                settled_at = time.monotonic()
            elif time.monotonic() - settled_at >= PROFILE_SETTLE_SECONDS:
                return
            time.sleep(1.0)
        raise TimeoutError(
            f"Kodi kept writing to {DATA_DIR}/userdata for the whole "
            f"{self.startup_timeout}s, so the profile never reached a state safe to "
            f"stop it in. Logcat tail:\n{self._logcat_tail()}"
        )

    def _prime_profile_dir(self) -> None:
        """Launches Kodi once, on defaults, so that it creates the profile files
        seeding then rewrites."""
        self._shell("am", "start", "-W", "-n", SPLASH_ACTIVITY)
        deadline = time.monotonic() + self.startup_timeout
        while time.monotonic() < deadline:
            if self._shell("test", "-f", GUISETTINGS, check=False).returncode == 0:
                self._wait_for_profile_settled(deadline)
                self._shell("am", "force-stop", PACKAGE)
                return
            time.sleep(1.0)
        raise TimeoutError(
            f"Kodi did not create {GUISETTINGS} within {self.startup_timeout}s, so "
            f"there is no profile to seed settings into. Logcat tail:\n"
            f"{self._logcat_tail()}"
        )

    # (serial, apk) pairs installed in this pytest session.
    _installed: set[tuple[str, Path]] = set()

    def _ensure_installed(self) -> None:
        key = (self._serial, self.apk_path)
        if key in AndroidKodiProcess._installed:
            return
        self._restart_adbd_as_root()
        # -g grants the runtime permissions; MANAGE_EXTERNAL_STORAGE is an app-op.
        self._adb("install", "-r", "-g", str(self.apk_path))
        self._shell("appops", "set", PACKAGE, "MANAGE_EXTERNAL_STORAGE", "allow")
        AndroidKodiProcess._installed.add(key)

    def _reset_userdata(self) -> None:
        # Database/ is kept: no test asserts on it, and recreating every database
        # lengthens each priming launch. toybox find has no -delete.
        self._shell(
            "sh",
            "-c",
            shlex.quote(
                f"cd {DATA_DIR}/userdata 2>/dev/null || exit 0; "
                'ls -A | grep -vx Database | while read -r entry; do rm -rf "$entry"; done'
            ),
            check=False,
        )

    def _seed_settings(self) -> None:
        self._overwrite_remote_file(
            GUISETTINGS,
            GUISETTINGS_TEMPLATE.format(port=DEVICE_WEBSERVER_PORT, screenshot_dir=SCREENSHOT_DIR),
        )

    def start(self) -> None:
        self._ensure_installed()
        # Only userdata, so earlier logs and screenshots stay for CI to collect.
        self._reset_userdata()
        self._prime_profile_dir()
        self._seed_settings()
        self._shell("am", "force-stop", PACKAGE)
        # Cleared after priming so crash_report() only sees the launch under test.
        self._adb("logcat", "-b", "crash", "-c", check=False)
        self._shell("am", "start", "-W", "-n", SPLASH_ACTIVITY)
        self._adb("forward", f"tcp:{self.port}", f"tcp:{DEVICE_WEBSERVER_PORT}")
        self._wait_for_port()

    def _wait_for_port(self) -> None:
        # Splash hands over to the native activity, so the process is no reliable
        # liveness signal; the logcat tail explains a timeout.
        wait_for_webserver(
            self.port,
            self.startup_timeout,
            still_running=lambda: True,
            diagnostics=lambda: f"Logcat tail:\n{self._logcat_tail()}",
        )

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi process that has already been asked to quit.

        adb exposes no exit code: 0 means Kodi went away in time, 1 that it had
        to be force-stopped. Crashes show in crash_report().
        """
        if wait_until_gone(self._is_running, timeout, interval=1.0):
            self.exit_code = 0
        else:
            self._shell("am", "force-stop", PACKAGE)
            self.exit_code = 1
        return self.exit_code

    def crash_report(self) -> str:
        """Android's crash log buffer since start(), if it mentions Kodi, else ""."""
        result = self._adb("logcat", "-d", "-b", "crash", check=False)
        if result.returncode != 0 or PACKAGE not in result.stdout:
            return ""
        return "\n".join(result.stdout.splitlines()[-80:])

    def read_log(self) -> str:
        data = self._read_remote_file(f"{DATA_DIR}/temp/kodi.log")
        return data.decode(errors="replace") if data is not None else ""

    def read_guisettings(self) -> str:
        """The on-device guisettings.xml as Kodi last wrote it - see KodiProcess."""
        data = self._read_remote_file(GUISETTINGS)
        return data.decode(errors="replace") if data is not None else ""

    def kill_if_running(self) -> None:
        if self._is_running():
            self._shell("am", "force-stop", PACKAGE)
        self._adb("forward", "--remove", f"tcp:{self.port}", check=False)
        shutil.rmtree(self._local_dir, ignore_errors=True)
