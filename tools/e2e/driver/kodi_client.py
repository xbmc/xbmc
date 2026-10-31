"""Minimal JSON-RPC over HTTP client for driving a running Kodi.

Request/response only; waiting for notifications (e.g. Player.OnPlay) would
need a WebSocket client.
"""

import itertools
import time
from typing import Any

import requests

# xbmc/guilib/WindowIDs.h / xbmc/input/WindowTranslator.cpp name->id table.
WINDOW_HOME = 10000
WINDOW_SETTINGS_MENU = 10004


class KodiJsonRpcError(RuntimeError):
    """Raised when Kodi's JSON-RPC endpoint returns an error response."""


class KodiJsonRpcClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 18080, timeout: float = 10.0):
        self._url = f"http://{host}:{port}/jsonrpc"
        self._timeout = timeout
        self._ids = itertools.count(1)

    def call(self, method: str, params: dict[str, Any] | None = None) -> Any:
        payload: dict[str, Any] = {
            "jsonrpc": "2.0",
            "method": method,
            "id": next(self._ids),
        }
        if params is not None:
            payload["params"] = params

        response = requests.post(self._url, json=payload, timeout=self._timeout)
        response.raise_for_status()
        body = response.json()

        if "error" in body:
            raise KodiJsonRpcError(f"{method} failed: {body['error']}")
        return body.get("result")

    def ping(self) -> Any:
        """Calls JSONRPC.Ping, which should return the literal string "pong"."""
        return self.call("JSONRPC.Ping")

    def quit(self) -> Any:
        """Asks Kodi to shut down cleanly via Application.Quit."""
        return self.call("Application.Quit")

    def execute_action(self, action: str) -> Any:
        """Calls Input.ExecuteAction, e.g. execute_action("screenshot").

        Returns once the action is queued, not once it is done: a screenshot,
        for one, is written by a background job.
        """
        return self.call("Input.ExecuteAction", {"action": action})

    def current_window_id(self) -> int:
        """The numeric id of the active GUI window."""
        result = self.call("GUI.GetProperties", {"properties": ["currentwindow"]})
        return result["currentwindow"]["id"]

    def wait_for_window(self, window_id: int, timeout: float) -> None:
        """Polls current_window_id() until it equals window_id.

        currentwindow changes when activation starts, before the window has
        loaded, so this proves navigation happened, not that anything was drawn.
        """
        deadline = time.monotonic() + timeout
        last_window_id = None
        while time.monotonic() < deadline:
            try:
                last_window_id = self.current_window_id()
                if last_window_id == window_id:
                    return
            except (KodiJsonRpcError, requests.exceptions.RequestException):
                pass
            time.sleep(0.5)
        raise TimeoutError(
            f"Window {window_id} not reached within {timeout}s "
            f"(last seen window id: {last_window_id})"
        )

    def activate_window(self, window: str, parameters: list[str] | None = None) -> Any:
        """Calls GUI.ActivateWindow; asynchronous like execute_action, so follow
        with wait_for_window()."""
        params: dict[str, Any] = {"window": window}
        if parameters:
            params["parameters"] = parameters
        return self.call("GUI.ActivateWindow", params)

    def get_setting(self, setting: str) -> Any:
        """Calls Settings.GetSettingValue and returns the setting's current value."""
        result = self.call("Settings.GetSettingValue", {"setting": setting})
        return result["value"]

    def set_setting(self, setting: str, value: Any) -> bool:
        """Calls Settings.SetSettingValue, returning whether the change was accepted."""
        return self.call("Settings.SetSettingValue", {"setting": setting, "value": value})
