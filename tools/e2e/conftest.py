import os
import pathlib
import shutil
import sys

import pytest

# Allow "from driver.xxx import yyy" regardless of the directory pytest is invoked from.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from driver.android_launcher import AndroidKodiProcess  # noqa: E402
from driver.assertions import assert_clean_shutdown  # noqa: E402
from driver.instance import KodiInstance, free_port  # noqa: E402
from driver.ios_launcher import IOSKodiProcess  # noqa: E402
from driver.kodi_client import KodiJsonRpcClient  # noqa: E402
from driver.launcher import KodiProcess  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_KODI_BINARY = REPO_ROOT / "build" / "kodi.bin"


def _make_instance(port: int) -> KodiInstance:
    # KODI_APK/KODI_APP select the adb/emulator or simctl/simulator launcher instead
    # of a local desktop binary - see driver/android_launcher.py and
    # driver/ios_launcher.py for why each platform needs its own launch/teardown/
    # test-isolation strategy rather than reusing KodiProcess.
    apk_path = os.environ.get("KODI_APK")
    app_path = os.environ.get("KODI_APP")
    if apk_path:
        return AndroidKodiProcess(apk_path, serial=os.environ.get("ANDROID_SERIAL"), port=port)
    if app_path:
        return IOSKodiProcess(
            app_path, device=os.environ.get("KODI_SIMULATOR_DEVICE", "booted"), port=port
        )

    # KODI_PROFILE_DIR drives an installed Kodi through KODI_DATA instead of -p; see
    # driver/launcher.py.
    proc = KodiProcess(
        os.environ.get("KODI_BINARY", DEFAULT_KODI_BINARY),
        port=port,
        profile_dir=os.environ.get("KODI_PROFILE_DIR"),
    )
    # Kodi's clean shutdown rewrites guisettings.xml, and NetworkServices::Start()'s
    # auth guard can flip services.webserver off - wipe just userdata (not the whole
    # profile) so that never leaks into the next test. Wiping the whole directory
    # would also delete screenshots/logs from earlier tests in the same run, leaving
    # nothing for CI to upload if a later test doesn't take one itself.
    shutil.rmtree(proc.profile_dir / "userdata", ignore_errors=True)
    return proc


@pytest.fixture
def kodi():
    """A running Kodi, shut down and checked at teardown.

    Teardown runs assert_clean_shutdown unless the scenario already shut Kodi down
    through it (exit_code is then set), so every scenario gets the exit-code, crash
    and fatal-log checks without having to remember them. A scenario that failed
    earlier still gets Kodi's shutdown checked, which reports as a teardown error
    next to the failure.
    """
    port = int(os.environ["KODI_PORT"]) if os.environ.get("KODI_PORT") else free_port()
    proc = _make_instance(port)
    try:
        proc.start()
    except BaseException:
        proc.kill_if_running()
        raise

    try:
        yield proc
        if proc.exit_code is None:
            assert_clean_shutdown(proc, KodiJsonRpcClient(port=proc.port))
    finally:
        proc.kill_if_running()
