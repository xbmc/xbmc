"""E2E test: window activation and the back stack, via JSON-RPC.

GUI.ActivateWindow and Input.Back, unlike directional keys, do not depend on the
skin's focus order.
"""

from driver.kodi_client import WINDOW_HOME, WINDOW_SETTINGS_MENU, KodiJsonRpcClient

HOME_WINDOW_TIMEOUT = 60.0
NAVIGATION_TIMEOUT = 15.0


def test_activate_window_and_back(client: KodiJsonRpcClient):
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    client.wait_for_window(WINDOW_HOME, HOME_WINDOW_TIMEOUT)

    client.activate_window("settings")
    client.wait_for_window(WINDOW_SETTINGS_MENU, NAVIGATION_TIMEOUT)

    client.call("Input.Back")
    client.wait_for_window(WINDOW_HOME, NAVIGATION_TIMEOUT)
