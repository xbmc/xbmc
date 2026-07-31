import os
import pathlib
import shutil
import sys

import pytest

# Allow "from driver.xxx import yyy" regardless of the directory pytest is invoked from.
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from driver.android_launcher import AndroidKodiProcess  # noqa: E402
from driver.launcher import KodiProcess  # noqa: E402

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
DEFAULT_KODI_BINARY = REPO_ROOT / "build" / "kodi.bin"


def _kodi_binary_path() -> pathlib.Path:
    return pathlib.Path(os.environ.get("KODI_BINARY", DEFAULT_KODI_BINARY))


@pytest.fixture
def kodi():
    # KODI_APK selects the adb/emulator launcher; see driver/android_launcher.py for
    # why that platform needs its own launch, teardown and isolation strategy.
    apk_path = os.environ.get("KODI_APK")
    if apk_path:
        proc = AndroidKodiProcess(apk_path, serial=os.environ.get("ANDROID_SERIAL"))
    else:
        proc = KodiProcess(_kodi_binary_path())
        # Kodi's clean shutdown rewrites guisettings.xml, and NetworkServices::Start()'s
        # auth guard can flip services.webserver off, so userdata must not carry over
        # between tests. Only userdata: wiping all of portable_data_dir would also drop
        # screenshots and logs that CI uploads for tests that don't take their own.
        shutil.rmtree(proc.portable_data_dir / "userdata", ignore_errors=True)

    try:
        proc.start()
        yield proc
    finally:
        proc.kill_if_running()
