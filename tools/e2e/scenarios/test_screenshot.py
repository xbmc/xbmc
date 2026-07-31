"""End-to-end rendering smoke test: launch Kodi, take a screenshot, sanity-check it.

This is not visual/pixel-regression testing (see docs/E2E-TESTING.md's Phase 2 for
that) - it only checks that something plausible got rendered at all, catching e.g. a
GL context that "succeeds" but draws nothing (a blank/solid-color frame).
"""

import time

from PIL import Image

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import WINDOW_HOME, KodiJsonRpcClient
from driver.launcher import KodiProcess

HOME_WINDOW_TIMEOUT = 60.0
# Generous: currentwindow flips to WINDOW_HOME the instant activation *starts*
# (GUIWindowManager.cpp adds it to the window history before sending the WINDOW_INIT
# message that actually parses Home.xml and loads its controls/textures), not once
# it's actually finished loading and drawn - so there's no reliable "done rendering"
# signal to poll for. Retry the screenshot itself instead, on a generous budget to
# absorb how slow the software-rendering fallback's first paint can be.
RENDER_TIMEOUT = 90.0
SCREENSHOT_TIMEOUT = 15.0  # per-attempt budget for one screenshot file to appear/settle
RETRY_INTERVAL = 2.0
MIN_EXPECTED_DIMENSION = 100  # sanity floor, well below any real Kodi window size


def _wait_for_new_screenshot(kodi: KodiProcess, existing: set, timeout: float):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        new_files = set(kodi.screenshot_dir.glob("*.png")) - existing
        if new_files:
            screenshot_name = new_files.pop().name
            # CScreenShot::TakeScreenshot writes the file (empty) before the background
            # CThumbnailWriter job fills it in, so wait for a stable, non-zero size
            # rather than returning the moment the (possibly still-empty) file appears.
            size = -1
            while time.monotonic() < deadline:
                # Re-resolved through kodi.screenshot_dir every poll, not hoisted: the
                # Android launcher's screenshot_dir re-copies the device's PNGs on each
                # access, and a file pulled mid-write would otherwise sit at its partial
                # size forever and read as settled.
                screenshot = kodi.screenshot_dir / screenshot_name
                current_size = screenshot.stat().st_size
                if current_size > 0 and current_size == size:
                    return screenshot
                size = current_size
                time.sleep(0.5)
            raise TimeoutError(
                f"{screenshot_name} appeared but never finished writing (stuck at "
                f"{size} bytes) within {timeout}s"
            )
        time.sleep(0.5)
    raise TimeoutError(f"No new screenshot appeared in {kodi.screenshot_dir} within {timeout}s")


def _capture_non_blank_screenshot(kodi: KodiProcess, client: KodiJsonRpcClient, timeout: float):
    deadline = time.monotonic() + timeout
    last_value = None
    while True:
        existing_screenshots = set(kodi.screenshot_dir.glob("*.png"))
        client.execute_action("screenshot")

        remaining = max(deadline - time.monotonic(), 1.0)
        screenshot_path = _wait_for_new_screenshot(
            kodi, existing_screenshots, min(SCREENSHOT_TIMEOUT, remaining)
        )

        with Image.open(screenshot_path) as image:
            image.verify()  # raises if the PNG is truncated/corrupt

        with Image.open(screenshot_path) as image:
            width, height = image.size
            assert width >= MIN_EXPECTED_DIMENSION and height >= MIN_EXPECTED_DIMENSION, (
                f"Screenshot {screenshot_path} is implausibly small ({width}x{height})"
            )

            # A single solid color (all-black being the classic symptom) means
            # something rendered a context but never actually drew the GUI into it.
            last_value = image.convert("L").getextrema()

        if last_value[0] != last_value[1]:
            return screenshot_path

        if time.monotonic() >= deadline:
            raise TimeoutError(
                f"No non-blank screenshot within {timeout}s - still a single solid "
                f"color (value {last_value[0]}) on the last attempt ({screenshot_path})"
            )
        time.sleep(RETRY_INTERVAL)


def test_screenshot_renders_non_blank_frame(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    client.wait_for_window(WINDOW_HOME, HOME_WINDOW_TIMEOUT)
    _capture_non_blank_screenshot(kodi, client, RENDER_TIMEOUT)

    assert_clean_shutdown(kodi, client)
