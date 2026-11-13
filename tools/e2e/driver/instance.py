"""The contract every launcher fulfils, and the polling helpers they share.

Scenarios only see a KodiInstance; conftest.py decides which launcher provides it.
"""

import socket
import time
from pathlib import Path
from typing import Callable, Protocol

import requests

from driver.kodi_client import KodiJsonRpcClient, KodiJsonRpcError


class KodiInstance(Protocol):
    port: int
    """Host-side port the JSON-RPC webserver is reachable on."""

    exit_code: int | None
    """Set by wait_for_exit(); None while Kodi has not been shut down through it."""

    def pull_screenshots(self) -> list[Path]:
        """Local copies of the PNGs Kodi's screenshot action has written so far."""

    def start(self) -> None:
        """Launches Kodi with the seeded profile and returns once JSON-RPC answers."""

    def wait_for_exit(self, timeout: float = 30.0) -> int:
        """Waits for a Kodi already asked to quit; 0 means it went away cleanly."""

    def read_log(self) -> str:
        """kodi.log as Kodi last wrote it, or "" if it never got created."""

    def read_guisettings(self) -> str:
        """The profile's guisettings.xml as Kodi last wrote it, or "" if absent."""

    def crash_report(self) -> str:
        """Platform crash evidence produced since start(), or "" if there is none.

        Android and the Simulator report exit code 0 as soon as the process is
        gone, so a crash only shows here.
        """

    def kill_if_running(self) -> None:
        """Forcibly stops Kodi if it is still up and releases launcher resources."""


def free_port() -> int:
    """An ephemeral TCP port that was free a moment ago.

    Kodi binds it later, so another process could take it first; acceptable for
    a test harness.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def wait_for_webserver(
    port: int,
    timeout: float,
    still_running: Callable[[], bool],
    diagnostics: Callable[[], str],
) -> None:
    """Polls JSONRPC.Ping until Kodi's webserver answers on port.

    A completed round trip is required: `adb forward` accepts connections before
    anything listens on the device side.
    """
    deadline = time.monotonic() + timeout
    client = KodiJsonRpcClient(port=port, timeout=5.0)
    while time.monotonic() < deadline:
        if not still_running():
            raise RuntimeError(
                f"Kodi exited while waiting for the webserver on port {port}.\n{diagnostics()}"
            )
        try:
            if client.ping() == "pong":
                return
        except (KodiJsonRpcError, requests.exceptions.RequestException):
            pass
        time.sleep(1.0)
    raise TimeoutError(
        f"Kodi webserver did not answer on port {port} within {timeout}s.\n{diagnostics()}"
    )


def wait_until_gone(is_running: Callable[[], bool], timeout: float, interval: float = 0.5) -> bool:
    """True if is_running() turned false within timeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if not is_running():
            return True
        time.sleep(interval)
    return not is_running()
