# -*- coding: utf-8 -*-
"""
chain_manager.py — Read and write the audiodsp.vsthost chain.json file.

Provides a Python interface to the VST plugin chain configuration that is
shared with the C++ audiodsp.vsthost ADSP addon.

License: GPL-2.0-or-later
"""
from __future__ import unicode_literals

import copy
import json
import os

import xbmc
import xbmcvfs


# Default chain structure when no chain.json exists yet
_DEFAULT_CHAIN = {
    'version': 1,
    'sampleRate': 44100.0,
    'numChannels': 2,
    'plugins': []
}


class ChainManager(object):
    """Manages the VST plugin chain configuration via chain.json."""

    def __init__(self, data_path):
        """
        :param data_path: Filesystem path to the audiodsp.vsthost addon data
                          directory (already translated from special://).
        """
        self._data_path = data_path
        self._chain_file = os.path.join(data_path, 'chain.json')

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def get_chain_paths(self):
        """Return a set of plugin file paths currently in the chain."""
        data = self._read()
        return set(entry['path'] for entry in data.get('plugins', []))

    def get_chain_plugins(self):
        """Return the list of plugin dicts currently in the chain."""
        data = self._read()
        return data.get('plugins', [])

    def add_plugin(self, path, fmt):
        """Append a plugin to the chain if not already present.

        :param path: Absolute filesystem path to the VST .dll or .vst3 file.
        :param fmt:  Plugin format string — ``'vst2'`` or ``'vst3'``.
        :returns: True on success.
        """
        data = self._read()
        # Avoid duplicates
        for entry in data['plugins']:
            if entry['path'] == path:
                return True
        data['plugins'].append({
            'path': path,
            'format': fmt,
            'bypassed': False,
            'state': ''
        })
        self._write(data)
        return True

    def remove_plugin(self, path):
        """Remove a plugin from the chain by its file path.

        :param path: Absolute filesystem path of the plugin to remove.
        """
        data = self._read()
        data['plugins'] = [p for p in data['plugins'] if p['path'] != path]
        self._write(data)

    def cleanup_missing(self):
        """Remove chain entries whose plugin files no longer exist on disk.

        :returns: Number of entries removed.
        """
        data = self._read()
        original_count = len(data['plugins'])
        data['plugins'] = [p for p in data['plugins']
                           if xbmcvfs.exists(p['path'])]
        removed = original_count - len(data['plugins'])
        if removed > 0:
            self._write(data)
        return removed

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _read(self):
        """Read chain.json; return default structure if missing/corrupt."""
        if not xbmcvfs.exists(self._chain_file):
            return copy.deepcopy(_DEFAULT_CHAIN)
        try:
            f = xbmcvfs.File(self._chain_file, 'r')
            try:
                content = f.read()
            finally:
                f.close()
            if not content:
                return copy.deepcopy(_DEFAULT_CHAIN)
            data = json.loads(content)
            if 'plugins' not in data:
                data['plugins'] = []
            return data
        except Exception:
            return copy.deepcopy(_DEFAULT_CHAIN)

    def _write(self, data):
        """Write chain.json, creating the directory if needed."""
        if not xbmcvfs.exists(self._data_path):
            xbmcvfs.mkdirs(self._data_path)
        f = xbmcvfs.File(self._chain_file, 'w')
        try:
            f.write(json.dumps(data, indent=2))
        finally:
            f.close()
