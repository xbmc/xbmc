"""Launches Kodi's Android build on an adb-connected device/emulator.

Android Kodi has one fixed profile and no portable-profile flag. Its CLI parser
(xbmc/application/AppParamParser.cpp) is never invoked on this platform:
CXBMCApp::Create() (xbmc/platform/android/activity/XBMCApp.cpp) constructs a default
CAppParams(), whose UserDirectoriesLocation is PLATFORM rather than PORTABLE
(xbmc/application/AppParams.h). So the profile always lives at $HOME/.kodi, with HOME
set to getExternalFilesDir("") by CXBMCApp::SetupEnv - external storage - falling back
to getDir(..., MODE_PRIVATE) on internal storage only when that returns null.

KODI_HOME/KODI_BIN_HOME is a different variable: SetupEnv's cacheDir, from
getFilesDir(), holding the read-only asset tree unzipped from the APK. It is always on
internal storage no matter where HOME lands, so paths under /data/user/0/<pkg>/files/
say nothing about where the profile is.

Reaching the profile from adb on API 30 is the constraint this launcher is built
around. Each of the three ways in fails differently:

  * `run-as` runs as the app's UID but re-executes inside adbd's mount namespace,
    which has no per-app view of /storage: "mkdir: '/storage/emulated': Permission
    denied". It reaches internal storage only.
  * uid shell is refused by scoped storage, even for reads: "adb: error: failed to
    stat remote object ... Permission denied".
  * root reaches the files, but anything root *creates* there goes through the lower
    filesystem rather than the storage FUSE daemon, landing labelled storage_file
    instead of media_rw_data_file. The app is then denied by SELinux on its own files
    ("avc: denied { getattr } ... scontext=u:r:untrusted_app ...
    tcontext=u:object_r:storage_file") and aborts on "unable to load settings".

Hence: root for access, but create nothing. Kodi is launched once to build its own
profile tree (_prime_profile_dir), and seeding rewrites file contents in place
(_overwrite_remote_file) so the inode, owner and SELinux label stay as Kodi made them.
Screenshots go to special://temp/ for the same reason - a directory Kodi already
created. Test isolation clears userdata and lets Kodi rebuild it.

Kodi's storage/microphone consent flow (tools/android/packaging/xbmc/src/
Splash.java.in) blocks first launch on an interactive MANAGE_EXTERNAL_STORAGE settings
screen. Pre-granting that app-op via `adb shell appops set` makes Splash's onCreate()
see Environment.isExternalStorageManager() as true and skip the whole dialog chain,
which also gates the RECORD_AUDIO prompt.
"""

from __future__ import annotations

import shlex
import shutil
import subprocess
import tempfile
import time
from pathlib import Path

import requests

from driver.kodi_client import KodiJsonRpcClient, KodiJsonRpcError
from driver.launcher import DEFAULT_WEBSERVER_PORT, GUISETTINGS_TEMPLATE

PACKAGE = "org.xbmc.kodi"
SPLASH_ACTIVITY = f"{PACKAGE}/{PACKAGE}.Splash"
# The external-storage HOME (see the module docstring). Kodi logs the path it
# actually resolved as "special://home/ is mapped to"; check that line in logcat if
# seeding appears to succeed but the webserver never comes up, which is what writing
# to the wrong profile looks like.
DATA_DIR = f"/storage/emulated/0/Android/data/{PACKAGE}/files/.kodi"
GUISETTINGS = f"{DATA_DIR}/userdata/guisettings.xml"
# special://temp/, which Kodi creates for itself and already writes kodi.log into.
# Reused as the screenshot target because this launcher must not create directories
# out here at all - see the module docstring.
SCREENSHOT_DIR = f"{DATA_DIR}/temp"
# How long userdata must go unchanged before Kodi is considered safe to stop during
# profile priming - see _wait_for_profile_settled.
PROFILE_SETTLE_SECONDS = 5.0


class AndroidKodiProcess:
    def __init__(
        self,
        apk_path: str | Path,
        serial: str | None = None,
        port: int = DEFAULT_WEBSERVER_PORT,
        # Generous: first launch also unzips the APK's assets/ tree into internal
        # storage (Splash.java.in's FillCache), which the desktop launchers never do.
        startup_timeout: float = 180.0,
    ):
        self.apk_path = Path(apk_path).resolve()
        if not self.apk_path.exists():
            raise FileNotFoundError(
                f"Kodi APK not found at {self.apk_path}. Build it first (`make apk`), "
                "or set the KODI_APK environment variable."
            )

        self.port = port
        self.startup_timeout = startup_timeout
        self._adb_prefix = ["adb"] + (["-s", serial] if serial else [])
        self._local_dir = Path(tempfile.mkdtemp(prefix="kodi-e2e-android-"))
        self._started = False

    def _adb(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [*self._adb_prefix, *args], check=check, capture_output=True, text=True
        )

    def _shell(self, *args: str, check: bool = True) -> subprocess.CompletedProcess:
        return self._adb("shell", *args, check=check)

    def _read_remote_file(self, remote_path: str) -> bytes | None:
        """Reads a file off the device as raw bytes.

        Uses `adb exec-out` rather than `adb shell`: plain `adb shell` allocates a pty
        that mangles binary content (e.g. screenshot PNGs) with newline translation,
        which `exec-out` avoids.
        """
        result = subprocess.run(
            [*self._adb_prefix, "exec-out", "cat", remote_path], capture_output=True
        )
        return result.stdout if result.returncode == 0 else None

    def _overwrite_remote_file(self, remote_path: str, content: str) -> None:
        """Replaces an existing device file's contents, keeping the file itself.

        Deliberately not an `adb push` straight to remote_path: that creates a new
        file, and a file root creates out on external storage is unreadable to the
        app (see the module docstring). Truncating one Kodi already made preserves
        its inode, owner and SELinux label. Staged through /data/local/tmp, which is
        plain internal storage with none of those constraints.
        """
        staging_path = f"/data/local/tmp/{Path(remote_path).name}"
        with tempfile.NamedTemporaryFile("w", delete=False) as f:
            f.write(content)
            local_path = f.name
        try:
            self._adb("push", local_path, staging_path)
            # Quoted so that adb's argv-rejoin hands the device's shell one command
            # with the redirect still attached to it.
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

    @property
    def screenshot_dir(self) -> Path:
        """A local mirror of the device's screenshot folder.

        Scenarios (e.g. test_screenshot.py) glob/stat this as an ordinary local
        directory - copy the device folder's PNGs fresh on every access rather than
        teaching the shared scenario code about adb. Only the PNGs, since
        SCREENSHOT_DIR is special://temp/ and pulling it wholesale would drag
        kodi.log along on every single glob. Harmless before Kodi has taken any
        (check=False swallows the empty glob).
        """
        local = self._local_dir / "screenshots"
        local.mkdir(parents=True, exist_ok=True)
        listing = self._shell(
            "sh", "-c", shlex.quote(f"ls -1 {SCREENSHOT_DIR}/*.png"), check=False
        )
        if listing.returncode == 0:
            for remote in listing.stdout.split():
                data = self._read_remote_file(remote)
                if data is not None:
                    (local / Path(remote).name).write_bytes(data)
        return local

    def _is_running(self) -> bool:
        result = self._shell("pidof", PACKAGE, check=False)
        return result.returncode == 0 and bool(result.stdout.strip())

    def _logcat_tail(self, lines: int = 60) -> str:
        result = self._adb("logcat", "-d", "-s", "Kodi:V", "*:F", check=False)
        if result.returncode != 0:
            return "(no logcat captured)"
        return "\n".join(result.stdout.splitlines()[-lines:])

    def _restart_adbd_as_root(self) -> None:
        """Restarts adbd as root, the only UID that can reach the profile at all.

        See the module docstring for why neither run-as nor uid shell can. Root is
        available on the userdebug emulator images this suite targets (google_apis,
        not google_apis_playstore) and on no retail device.
        """
        self._adb("root", check=False)
        self._adb("wait-for-device", check=False)

    def _wait_for_profile_settled(self, deadline: float) -> None:
        """Waits until nothing under userdata has changed for PROFILE_SETTLE_SECONDS.

        guisettings.xml appears early in startup, but Kodi keeps creating the profile
        after that - the databases in particular. Stopping in between leaves e.g.
        MyVideos*.db present but without its schema, and the next launch dies on
        "no such table: version" and never reaches the webserver.
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
        """Launches Kodi once so that it creates its own profile tree.

        Nothing else may create anything under it (module docstring), so every file
        this launcher touches afterwards has to be one Kodi made here first. Kodi
        with no guisettings.xml starts up on defaults with the webserver off, which
        is all this needs.
        """
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

    def _install(self) -> None:
        # -g grants the normal runtime permissions the manifest declares (RECORD_AUDIO,
        # legacy WRITE_EXTERNAL_STORAGE) at install time; MANAGE_EXTERNAL_STORAGE is a
        # special app-op, not a runtime permission, so it needs the separate appops
        # call below regardless.
        self._adb("install", "-r", "-g", str(self.apk_path))

    def _grant_permissions(self) -> None:
        self._shell("appops", "set", PACKAGE, "MANAGE_EXTERNAL_STORAGE", "allow")

    def _reset_userdata(self) -> None:
        # Database/ is kept. Deleting it means Kodi has to recreate every database on
        # the next priming launch, which is the work _wait_for_profile_settled then has
        # to wait out; the databases hold nothing this suite asserts on. Iterated in the
        # shell rather than with `find -delete` since Android's toybox find is minimal.
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
            GUISETTINGS_TEMPLATE.format(port=self.port, screenshot_dir=SCREENSHOT_DIR),
        )

    def start(self) -> None:
        self._restart_adbd_as_root()
        self._install()
        self._grant_permissions()
        # Only userdata, so screenshots and logs from a previous run stay available for
        # CI to collect. _prime_profile_dir then has Kodi rebuild what was cleared,
        # which is the only way those files come back with an owner and SELinux label
        # the app can use.
        self._reset_userdata()
        self._prime_profile_dir()
        self._seed_settings()
        self._shell("am", "force-stop", PACKAGE)
        self._shell("am", "start", "-W", "-n", SPLASH_ACTIVITY)
        self._adb("forward", f"tcp:{self.port}", f"tcp:{self.port}")
        self._started = True
        self._wait_for_port()

    def _wait_for_port(self) -> None:
        """Waits for the JSON-RPC webserver to actually answer requests.

        A TCP connect proves nothing here: `adb forward`'s local listener accepts
        connections as soon as the forward rule exists, whether or not anything is
        listening on the device side. It surfaces as "Remote end closed connection
        without response" while Kodi is still unzipping assets in Splash. A JSON-RPC
        round trip only succeeds once the webserver is serving.
        """
        deadline = time.monotonic() + self.startup_timeout
        client = KodiJsonRpcClient(port=self.port, timeout=5.0)
        while time.monotonic() < deadline:
            try:
                if client.ping() == "pong":
                    return
            except (KodiJsonRpcError, requests.exceptions.RequestException):
                pass
            time.sleep(1.0)
        raise TimeoutError(
            f"Kodi webserver did not respond on port {self.port} within "
            f"{self.startup_timeout}s. Logcat tail:\n{self._logcat_tail()}"
        )

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi process that has already been asked to quit.

        Android has no process return code accessible over adb, so this reports 0
        once the process actually disappears within the deadline and 1 otherwise
        (after force-stopping it), mirroring the POSIX launcher's exit-code contract
        closely enough for assert_clean_shutdown to work unmodified.
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if not self._is_running():
                return 0
            time.sleep(1.0)
        self._shell("am", "force-stop", PACKAGE)
        return 1

    def read_log(self) -> str:
        data = self._read_remote_file(f"{DATA_DIR}/temp/kodi.log")
        return data.decode(errors="replace") if data is not None else ""

    def read_guisettings(self) -> str:
        """The on-device guisettings.xml as Kodi last wrote it - see KodiProcess."""
        data = self._read_remote_file(GUISETTINGS)
        return data.decode(errors="replace") if data is not None else ""

    def kill_if_running(self) -> None:
        if self._started and self._is_running():
            self._shell("am", "force-stop", PACKAGE)
        self._adb("forward", "--remove", f"tcp:{self.port}", check=False)
        shutil.rmtree(self._local_dir, ignore_errors=True)
