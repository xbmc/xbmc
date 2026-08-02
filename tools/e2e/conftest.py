import os
import pathlib
import shutil
import sys

import pytest

# Allow "from driver.xxx import yyy" regardless of the directory pytest is invoked from.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from driver.android_launcher import AndroidKodiProcess  # noqa: E402
from driver.ios_launcher import IOSKodiProcess  # noqa: E402
from driver.launcher import KodiProcess  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_KODI_BINARY = REPO_ROOT / "build" / "kodi.bin"


def _kodi_binary_path() -> pathlib.Path:
    return pathlib.Path(os.environ.get("KODI_BINARY", DEFAULT_KODI_BINARY))


@pytest.fixture
def kodi():
    # KODI_APK/KODI_APP select the adb/emulator or simctl/simulator launcher instead
    # of a local desktop binary - see driver/android_launcher.py and
    # driver/ios_launcher.py for why each platform needs its own launch/teardown/
    # test-isolation strategy rather than reusing KodiProcess.
    apk_path = os.environ.get("KODI_APK")
    app_path = os.environ.get("KODI_APP")
    if apk_path:
        proc = AndroidKodiProcess(apk_path, serial=os.environ.get("ANDROID_SERIAL"))
    elif app_path:
        proc = IOSKodiProcess(app_path, device=os.environ.get("KODI_SIMULATOR_DEVICE", "booted"))
    else:
        proc = KodiProcess(_kodi_binary_path())
        # Kodi's clean shutdown rewrites guisettings.xml, and NetworkServices::Start()'s
        # auth guard can flip services.webserver off - wipe just userdata (not all of
        # portable_data_dir) so that never leaks into the next test. Wiping the whole
        # directory would also delete screenshots/logs from earlier tests in the same
        # run, leaving nothing for CI to upload if a later test doesn't take one itself.
        shutil.rmtree(proc.portable_data_dir / "userdata", ignore_errors=True)

    try:
        proc.start()
        yield proc
    finally:
        proc.kill_if_running()
