"""End-to-end smoke test: launch Kodi, ping it over JSON-RPC, quit cleanly.

This is the minimal first slice of docs/E2E-TESTING.md: it doesn't touch playback,
navigation, or any GUI interaction, only confirms the built binary starts up, the
JSON-RPC webserver responds, and it shuts down cleanly without fatal errors.
"""

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import KodiJsonRpcClient
from driver.launcher import KodiProcess


def test_startup_ping_and_quit(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)

    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    assert_clean_shutdown(kodi, client)
