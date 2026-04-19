# -*- coding: utf-8 -*-
"""
editor_bridge.py — Named pipe client for the C++ EditorBridge.

Communicates with the audiodsp.vsthost C++ addon's named pipe server to
open and close native VST editor windows.

License: GPL-2.0-or-later
"""
from __future__ import unicode_literals

import json
import os
import sys

import xbmc

# Named pipe constants
PIPE_NAME = r'\\.\pipe\kodi_vsthost_editor'
PIPE_TIMEOUT_MS = 3000

# Detect platform — native pipe client only works on Windows
_IS_WINDOWS = (sys.platform == 'win32')


def _log(msg, level=xbmc.LOGDEBUG):
    xbmc.log('[editor_bridge] %s' % msg, level)


def _send_command(cmd_dict):
    """Send a JSON command to the C++ EditorBridge and return the response dict.

    Returns None if the bridge is unreachable or an error occurs.
    """
    if not _IS_WINDOWS:
        _log('Named pipe IPC is only available on Windows', xbmc.LOGWARNING)
        return None

    try:
        import ctypes
        from ctypes import wintypes

        kernel32 = ctypes.windll.kernel32

        GENERIC_READ  = 0x80000000
        GENERIC_WRITE = 0x40000000
        OPEN_EXISTING = 3
        INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

        # Try to open the named pipe
        pipe_name_w = PIPE_NAME
        handle = kernel32.CreateFileW(
            pipe_name_w,
            GENERIC_READ | GENERIC_WRITE,
            0,          # no sharing
            None,       # default security
            OPEN_EXISTING,
            0,          # default attributes
            None)

        if handle == INVALID_HANDLE_VALUE:
            _log('Cannot connect to EditorBridge pipe (addon not loaded?)')
            return None

        try:
            # Write the command
            msg = json.dumps(cmd_dict) + '\n'
            msg_bytes = msg.encode('utf-8')
            bytes_written = wintypes.DWORD(0)
            kernel32.WriteFile(
                handle,
                msg_bytes,
                len(msg_bytes),
                ctypes.byref(bytes_written),
                None)

            # Read the response
            buf_size = 4096
            buf = ctypes.create_string_buffer(buf_size)
            bytes_read = wintypes.DWORD(0)
            kernel32.ReadFile(
                handle,
                buf,
                buf_size - 1,
                ctypes.byref(bytes_read),
                None)

            if bytes_read.value > 0:
                response = buf.value[:bytes_read.value].decode('utf-8').strip()
                return json.loads(response)

            return None

        finally:
            kernel32.CloseHandle(handle)

    except Exception as exc:
        _log('Error communicating with EditorBridge: %s' % exc, xbmc.LOGWARNING)
        return None


def open_editor(plugin_path):
    """Request the C++ addon to open a native VST editor window.

    :param plugin_path: Absolute filesystem path to the VST plugin file.
    :returns: A dict with the response, or None if the bridge is unreachable.
              On success: ``{'status': 'ok', 'cmd': 'open', 'hasEditor': True/False}``
    """
    return _send_command({'cmd': 'open', 'path': plugin_path})


def close_editor(plugin_path):
    """Request the C++ addon to close a native VST editor window.

    :param plugin_path: Absolute filesystem path to the VST plugin file.
    :returns: A dict with the response, or None if the bridge is unreachable.
    """
    return _send_command({'cmd': 'close', 'path': plugin_path})


def close_all_editors():
    """Request the C++ addon to close all open VST editor windows."""
    return _send_command({'cmd': 'close_all'})


def ping():
    """Check if the EditorBridge is reachable.

    :returns: True if the bridge responded, False otherwise.
    """
    result = _send_command({'cmd': 'ping'})
    return result is not None and result.get('status') == 'ok'
