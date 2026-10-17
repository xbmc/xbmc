"""Shared post-scenario assertions.

The `kodi` fixture runs assert_clean_shutdown at teardown unless the scenario
already did, so a regression that crashes or hangs Kodi fails regardless of which
scenario was running when it happened.
"""

from __future__ import annotations

import re

import requests

from driver.instance import KodiInstance
from driver.kodi_client import KodiJsonRpcClient

# LOGFATAL prints as either "FATAL" or "critical", depending on how spdlog was built:
# xbmc/utils/log.h's SPDLOG_LEVEL_NAMES override only reaches a header-only spdlog, and
# cmake/modules/FindSpdlog.cmake sets SPDLOG_COMPILED_LIB, which keeps spdlog's own
# names. Kodi's own unit test accepts both spellings too (xbmc/utils/test/Testlog.cpp).
#
# Anchored on the level field of the log pattern ("%7l <%n>: %v", xbmc/utils/log.cpp):
# "critical" is also an ordinary word in log *messages* ("critical section"), which a
# loose search would flag.
FATAL_LOG_LINE = re.compile(r"\b(?:FATAL|CRITICAL)\s+<[^>]*>:", re.IGNORECASE)


def assert_clean_shutdown(
    kodi: KodiInstance, client: KodiJsonRpcClient, timeout: float = 30.0
) -> None:
    """Quits Kodi and asserts it exited cleanly: code 0, no crash, no fatal log lines."""
    try:
        client.quit()
    except requests.exceptions.RequestException:
        # Kodi tears down the webserver as part of quitting, so the HTTP response to
        # the quit call itself may never arrive cleanly - that's expected.
        pass

    exit_code = kodi.wait_for_exit(timeout=timeout)
    log_text = kodi.read_log()

    assert exit_code == 0, (
        f"Kodi did not exit cleanly (code {exit_code}). Log tail:\n"
        + "\n".join(log_text.splitlines()[-40:])
    )

    crash = kodi.crash_report()
    assert not crash, f"Kodi crashed:\n{crash}"

    fatal_lines = [line for line in log_text.splitlines() if FATAL_LOG_LINE.search(line)]
    assert not fatal_lines, f"Found fatal-level log line(s): {fatal_lines}"
