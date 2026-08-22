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
# A rendered Estuary home screen measures above 90% by _painted_fraction; a frame
# holding nothing but a mouse cursor measures 0.03%. 5% sits between the two with room
# either side for a darker skin or a busier cursor.
MIN_PAINTED_FRACTION = 0.05


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


def _painted_fraction(image: Image.Image) -> float:
    """Fraction of pixels that differ from the most common one.

    A GUI that drew nothing is not necessarily one flat colour: a frame that is 99.97%
    black with a mouse cursor in it still spans luminance 0 to 255, so "more than one
    colour" accepts it. Measured on real frames, this separates cleanly - an Estuary
    home screen sits above 90%, a cursor on black at 0.03%.
    """
    histogram = image.convert("L").histogram()
    total = sum(histogram)
    return (total - max(histogram)) / total


def _capture_non_blank_screenshot(kodi: KodiProcess, client: KodiJsonRpcClient, timeout: float):
    deadline = time.monotonic() + timeout
    last_failure = None
    while True:
        existing_screenshots = set(kodi.screenshot_dir.glob("*.png"))
        client.execute_action("screenshot")

        remaining = max(deadline - time.monotonic(), 1.0)
        try:
            screenshot_path = _wait_for_new_screenshot(
                kodi, existing_screenshots, min(SCREENSHOT_TIMEOUT, remaining)
            )
        except TimeoutError as error:
            # A screenshot dispatched while Home.xml is still loading may produce no
            # file at all (see RENDER_TIMEOUT) - one of the outcomes this loop exists to
            # absorb, so spend the whole budget on it rather than one attempt's share.
            last_failure = str(error)
        else:
            with Image.open(screenshot_path) as image:
                image.verify()  # raises if the PNG is truncated/corrupt

            with Image.open(screenshot_path) as image:
                width, height = image.size
                # Not retried, unlike a blank or missing frame: a real window that came
                # up at an implausible size will still be that size on the next attempt.
                assert width >= MIN_EXPECTED_DIMENSION and height >= MIN_EXPECTED_DIMENSION, (
                    f"Screenshot {screenshot_path} is implausibly small ({width}x{height})"
                )

                painted = _painted_fraction(image)

            if painted >= MIN_PAINTED_FRACTION:
                return screenshot_path

            last_failure = (
                f"only {painted:.2%} of the last screenshot ({screenshot_path}) differs "
                f"from its background, against a {MIN_PAINTED_FRACTION:.0%} threshold"
            )

        if time.monotonic() >= deadline:
            raise TimeoutError(
                f"Kodi never rendered a frame within {timeout}s - {last_failure}"
            )
        time.sleep(RETRY_INTERVAL)


def test_screenshot_renders_non_blank_frame(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    client.wait_for_window(WINDOW_HOME, HOME_WINDOW_TIMEOUT)
    _capture_non_blank_screenshot(kodi, client, RENDER_TIMEOUT)

    assert_clean_shutdown(kodi, client)
