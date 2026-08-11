"""E2E test: a setting change made via JSON-RPC actually persists.

Uses lookandfeel.enablerssfeeds as the round-trip subject: a plain boolean with no
dependents and no effect on the test harness itself, unlike e.g. the webserver
settings this whole suite depends on to be able to connect at all.
"""

import xml.etree.ElementTree as ET

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import KodiJsonRpcClient
from driver.launcher import KodiProcess

SETTING = "lookandfeel.enablerssfeeds"


def _saved_setting_value(guisettings_xml: str, setting_id: str) -> str | None:
    """The value CSettingsManager::Serialize wrote for one setting, or None if absent.

    Serialize (xbmc/settings/lib/SettingsManager.cpp) writes every non-reference,
    non-action setting as <setting id="...">value</setting>, so a missing element means
    Kodi never got as far as saving its settings rather than that this one was skipped.
    """
    root = ET.fromstring(guisettings_xml)
    element = root.find(f'./setting[@id="{setting_id}"]')
    return element.text if element is not None else None


def test_setting_value_roundtrip(kodi: KodiProcess):
    client = KodiJsonRpcClient(port=kodi.port)
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    default_value = client.get_setting(SETTING)
    assert default_value is False, f"Expected default {SETTING}=False, got {default_value!r}"

    assert client.set_setting(SETTING, True) is True, f"{SETTING} change was rejected"

    updated_value = client.get_setting(SETTING)
    assert updated_value is True, f"{SETTING} was not applied: expected True, got {updated_value!r}"

    assert_clean_shutdown(kodi, client)

    # The persistence half of the round trip, and the reason this test shuts Kodi down
    # mid-scenario rather than just asserting the read-back above: that read-back is
    # served by the same running instance that accepted the write, so it holds even if
    # the setting never reaches guisettings.xml.
    guisettings_xml = kodi.read_guisettings()
    assert guisettings_xml, "Kodi wrote no guisettings.xml, so nothing was persisted"

    saved_value = _saved_setting_value(guisettings_xml, SETTING)
    assert saved_value == "true", (
        f"{SETTING} did not persist to guisettings.xml: expected 'true', got {saved_value!r}"
    )
