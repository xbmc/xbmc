"""End-to-end rendering smoke test: launch Kodi, take a screenshot, sanity-check it.

Not pixel-regression testing: it only checks that something plausible was
rendered, catching e.g. a GL context that "succeeds" but draws nothing.
"""

import time
from pathlib import Path

from PIL import Image

from driver.kodi_client import WINDOW_HOME, KodiJsonRpcClient
from driver.instance import KodiInstance

HOME_WINDOW_TIMEOUT = 60.0
# currentwindow turns to home before Home.xml has loaded and drawn, and nothing
# signals the first paint, so the screenshot is retried on a budget that also
# absorbs software rendering's slow first frame.
RENDER_TIMEOUT = 90.0
SCREENSHOT_TIMEOUT = 15.0  # per-attempt budget for one screenshot file to appear/settle
RETRY_INTERVAL = 2.0
MIN_EXPECTED_DIMENSION = 100  # sanity floor, well below any real Kodi window size
# An Estuary home screen measures above 90% by _painted_fraction, a frame holding
# only a mouse cursor 0.03%.
MIN_PAINTED_FRACTION = 0.05


def _wait_for_new_screenshot(kodi: KodiInstance, existing: set[str], timeout: float) -> Path:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        new_files = [p for p in kodi.pull_screenshots() if p.name not in existing]
        if new_files:
            screenshot_name = new_files[0].name
            # The file is created empty and filled in by a background job, so wait
            # for a stable, non-zero size.
            size = -1
            while time.monotonic() < deadline:
                screenshot = next(
                    (p for p in kodi.pull_screenshots() if p.name == screenshot_name), None
                )
                current_size = screenshot.stat().st_size if screenshot else 0
                if screenshot and current_size > 0 and current_size == size:
                    return screenshot
                size = current_size
                time.sleep(0.5)
            raise TimeoutError(
                f"{screenshot_name} appeared but never finished writing (stuck at "
                f"{size} bytes) within {timeout}s"
            )
        time.sleep(0.5)
    raise TimeoutError(f"No new screenshot appeared within {timeout}s")


def _painted_fraction(image: Image.Image) -> float:
    """Fraction of pixels that differ from the most common one.

    "More than one colour" is not enough: a black frame with a mouse cursor in
    it spans the full luminance range.
    """
    histogram = image.convert("L").histogram()
    total = sum(histogram)
    return (total - max(histogram)) / total


def _capture_non_blank_screenshot(kodi: KodiInstance, client: KodiJsonRpcClient, timeout: float):
    deadline = time.monotonic() + timeout
    last_failure = None
    while True:
        existing_screenshots = {p.name for p in kodi.pull_screenshots()}
        client.execute_action("screenshot")

        remaining = max(deadline - time.monotonic(), 1.0)
        try:
            screenshot_path = _wait_for_new_screenshot(
                kodi, existing_screenshots, min(SCREENSHOT_TIMEOUT, remaining)
            )
        except TimeoutError as error:
            # A screenshot taken while Home.xml is still loading may never be
            # written; retry until the overall budget runs out.
            last_failure = str(error)
        else:
            with Image.open(screenshot_path) as image:
                image.verify()  # raises if the PNG is truncated/corrupt

            with Image.open(screenshot_path) as image:
                width, height = image.size
                # Not retried: a window of the wrong size stays that size.
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


def test_screenshot_renders_non_blank_frame(kodi: KodiInstance, client: KodiJsonRpcClient):
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    client.wait_for_window(WINDOW_HOME, HOME_WINDOW_TIMEOUT)
    _capture_non_blank_screenshot(kodi, client, RENDER_TIMEOUT)
