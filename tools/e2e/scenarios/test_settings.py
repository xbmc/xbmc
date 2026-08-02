"""E2E test: a setting change made via JSON-RPC actually persists.

Uses lookandfeel.enablerssfeeds as the round-trip subject: a plain boolean with no
dependents and no effect on the test harness itself, unlike e.g. the webserver
settings this whole suite depends on to be able to connect at all.
"""

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import KodiJsonRpcClient
from driver.launcher import KodiProcess

SETTING = "lookandfeel.enablerssfeeds"


def test_setting_value_roundtrip(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    default_value = client.get_setting(SETTING)
    assert default_value is False, f"Expected default {SETTING}=False, got {default_value!r}"

    assert client.set_setting(SETTING, True) is True, f"{SETTING} change was rejected"

    updated_value = client.get_setting(SETTING)
    assert updated_value is True, f"{SETTING} did not persist: expected True, got {updated_value!r}"

    assert_clean_shutdown(kodi, client)
