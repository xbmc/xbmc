"""Shared post-scenario assertions, used by every scenario that reaches a running GUI.

Kept separate from individual scenarios so a regression that crashes or hangs Kodi
fails loudly regardless of which scenario happened to be running when it did.
"""

from __future__ import annotations

import re

import requests

from driver.kodi_client import KodiJsonRpcClient
from driver.launcher import KodiProcess

FATAL_LOG_LINE = re.compile(r"\bFATAL\b")


def assert_clean_shutdown(
    kodi: KodiProcess, client: KodiJsonRpcClient, timeout: float = 30.0
) -> None:
    """Quits Kodi and asserts it exited cleanly (code 0, no FATAL log lines)."""
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

    fatal_lines = [line for line in log_text.splitlines() if FATAL_LOG_LINE.search(line)]
    assert not fatal_lines, f"Found FATAL log line(s): {fatal_lines}"
