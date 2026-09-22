"""End-to-end smoke test: launch Kodi and ping it over JSON-RPC.

The minimal slice of docs/E2E-TESTING.md: no playback, navigation or GUI
interaction, only that the built binary starts and the JSON-RPC webserver answers.
The clean quit is checked by the fixture's teardown.
"""

from driver.instance import KodiInstance
from driver.kodi_client import KodiJsonRpcClient


def test_startup_ping_and_quit(kodi: KodiInstance):
    client = KodiJsonRpcClient(port=kodi.port)

    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"
