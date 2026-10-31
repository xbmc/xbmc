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

E2E_ROOT = pathlib.Path(__file__).resolve().parent
DEFAULT_KODI_BINARY = E2E_ROOT.parent.parent / "build" / "kodi.bin"
ARTIFACTS_DIR = pathlib.Path(os.environ.get("KODI_E2E_ARTIFACTS", E2E_ROOT / "artifacts"))


def _make_instance(port: int, test_name: str) -> KodiInstance:
    # KODI_APK and KODI_APP select the Android and Simulator launchers.
    apk_path = os.environ.get("KODI_APK")
    app_path = os.environ.get("KODI_APP")
    if apk_path:
        return AndroidKodiProcess(apk_path, serial=os.environ.get("ANDROID_SERIAL"), port=port)
    if app_path:
        return IOSKodiProcess(
            app_path, device=os.environ.get("KODI_SIMULATOR_DEVICE", "booted"), port=port
        )

    # KODI_PROFILE_DIR gives each test its own profile under it, for an installed
    # Kodi; the portable profile is at a fixed path, so it is wiped instead.
    profile_root = os.environ.get("KODI_PROFILE_DIR")
    proc = KodiProcess(
        os.environ.get("KODI_BINARY", DEFAULT_KODI_BINARY),
        port=port,
        profile_dir=pathlib.Path(profile_root) / test_name if profile_root else None,
    )
    shutil.rmtree(proc.profile_dir, ignore_errors=True)
    return proc


def _collect_artifacts(proc: KodiInstance, test_name: str) -> None:
    """Keeps the test's log, process output and screenshots under ARTIFACTS_DIR.

    Best effort: a launcher that failed early may not know its paths yet, and that
    must not hide the failure itself.
    """
    dest = ARTIFACTS_DIR / test_name
    dest.mkdir(parents=True, exist_ok=True)
    try:
        log = proc.read_log()
        if log:
            (dest / "kodi.log").write_text(log)
        process_output = getattr(proc, "process_output_path", None)
        if process_output is not None and process_output.exists():
            shutil.copy2(process_output, dest)
        for screenshot in proc.pull_screenshots():
            shutil.copy2(screenshot, dest)
    except Exception as error:  # noqa: BLE001
        print(f"Could not collect artifacts for {test_name}: {error}", file=sys.stderr)


@pytest.fixture
def kodi(request):
    """A running Kodi, shut down and checked at teardown.

    Teardown runs assert_clean_shutdown unless the scenario already did
    (exit_code is then set). A scenario that failed still gets the shutdown
    checked, reported as a teardown error next to the failure.
    """
    port = int(os.environ["KODI_PORT"]) if os.environ.get("KODI_PORT") else free_port()
    proc = _make_instance(port, request.node.name)
    try:
        proc.start()
        yield proc
        if proc.exit_code is None:
            assert_clean_shutdown(proc, KodiJsonRpcClient(port=proc.port))
    finally:
        _collect_artifacts(proc, request.node.name)
        proc.kill_if_running()


@pytest.fixture
def client(kodi: KodiInstance) -> KodiJsonRpcClient:
    return KodiJsonRpcClient(port=kodi.port)
