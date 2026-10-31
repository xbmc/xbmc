"""Shared post-scenario assertions.

The `kodi` fixture runs assert_clean_shutdown at teardown unless the scenario
already did, so a crash or hang fails whichever scenario was running.
"""

import re

import requests

from driver.instance import KodiInstance
from driver.kodi_client import KodiJsonRpcClient

# LOGFATAL prints as "FATAL" or "critical": the SPDLOG_LEVEL_NAMES override in
# xbmc/utils/log.h only reaches a header-only spdlog, and with
# SPDLOG_COMPILED_LIB (cmake/modules/FindSpdlog.cmake) spdlog keeps its own
# names. Anchored on the level field of the log pattern
# (xbmc/utils/log.cpp), since "critical" also appears in ordinary messages.
FATAL_LOG_LINE = re.compile(r"\b(?:FATAL|CRITICAL)\s+<[^>]*>:", re.IGNORECASE)


def assert_clean_shutdown(
    kodi: KodiInstance, client: KodiJsonRpcClient, timeout: float = 30.0
) -> None:
    """Quits Kodi and asserts a clean exit: code 0, no crash, a kodi.log without fatal lines."""
    try:
        client.quit()
    except requests.exceptions.RequestException:
        # Kodi tears down the webserver while quitting, so the reply may not arrive.
        pass

    exit_code = kodi.wait_for_exit(timeout=timeout)
    log_text = kodi.read_log()

    assert exit_code == 0, (
        f"Kodi did not exit cleanly (code {exit_code}). Log tail:\n"
        + "\n".join(log_text.splitlines()[-40:])
    )

    crash = kodi.crash_report()
    assert not crash, f"Kodi crashed:\n{crash}"

    assert log_text, "kodi.log is missing or empty"

    fatal_lines = [line for line in log_text.splitlines() if FATAL_LOG_LINE.search(line)]
    assert not fatal_lines, f"Found fatal-level log line(s): {fatal_lines}"
