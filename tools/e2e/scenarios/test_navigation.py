"""E2E test: GUI window navigation via JSON-RPC.

Drives navigation through GUI.ActivateWindow/Input.Back rather than replaying
Down/Select keypresses: focus order within a skin's layout is skin- and
version-specific, so scripting blind directional navigation would make this test
fragile in a way unrelated to what it's meant to catch (see
_capture_non_blank_screenshot's docstring in test_screenshot.py for the same class of
issue with currentwindow timing). GUI.ActivateWindow and Input.Back are the
skin-independent, documented JSON-RPC equivalents of "open this window" and "go
back" - what matters here is whether window activation and the back-stack work, not
which skin element happens to be focused first.
"""

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import WINDOW_HOME, WINDOW_SETTINGS_MENU, KodiJsonRpcClient
from driver.launcher import KodiProcess

HOME_WINDOW_TIMEOUT = 60.0
NAVIGATION_TIMEOUT = 15.0


def test_activate_window_and_back(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    client.wait_for_window(WINDOW_HOME, HOME_WINDOW_TIMEOUT)

    client.activate_window("settings")
    client.wait_for_window(WINDOW_SETTINGS_MENU, NAVIGATION_TIMEOUT)

    client.call("Input.Back")
    client.wait_for_window(WINDOW_HOME, NAVIGATION_TIMEOUT)

    assert_clean_shutdown(kodi, client)
