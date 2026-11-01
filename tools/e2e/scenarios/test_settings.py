"""E2E test: a setting changed over JSON-RPC persists to guisettings.xml.

filelists.showhidden is a plain boolean that only changes how file lists are
shown, so turning it on has no side effects.
"""

import xml.etree.ElementTree as ET

from driver.assertions import assert_clean_shutdown
from driver.kodi_client import KodiJsonRpcClient
from driver.instance import KodiInstance

SETTING = "filelists.showhidden"


def _saved_setting_value(guisettings_xml: str, setting_id: str) -> str | None:
    """The value Kodi saved for setting_id, or None if absent.

    CSettingsManager::Serialize() writes every saveable setting, so None means
    the settings were never saved.
    """
    root = ET.fromstring(guisettings_xml)
    element = root.find(f'./setting[@id="{setting_id}"]')
    return element.text if element is not None else None


def test_setting_value_roundtrip(kodi: KodiInstance, client: KodiJsonRpcClient):
    assert client.ping() == "pong", "JSONRPC.Ping did not return the expected 'pong'"

    default_value = client.get_setting(SETTING)
    assert default_value is False, f"Expected default {SETTING}=False, got {default_value!r}"

    assert client.set_setting(SETTING, True) is True, f"{SETTING} change was rejected"

    updated_value = client.get_setting(SETTING)
    assert updated_value is True, f"{SETTING} was not applied: expected True, got {updated_value!r}"

    assert_clean_shutdown(kodi, client)

    # Shut down here rather than in the fixture: the read-back above only proves
    # the in-memory value, and Kodi saves settings on a clean shutdown.
    guisettings_xml = kodi.read_guisettings()
    assert guisettings_xml, "Kodi wrote no guisettings.xml, so nothing was persisted"

    saved_value = _saved_setting_value(guisettings_xml, SETTING)
    assert saved_value == "true", (
        f"{SETTING} did not persist to guisettings.xml: expected 'true', got {saved_value!r}"
    )
