# -*- coding: utf-8 -*-
"""
plugin_scanner.py — VST plugin discovery for the VST Manager addon.

Reads the plugin_cache.json produced by vstscanner.exe and/or scans
a directory for VST plugin files (.dll, .vst3).

License: GPL-2.0-or-later
"""
from __future__ import unicode_literals

import json
import os

import xbmc
import xbmcvfs


def _translate_path(path):
    """Translate a Kodi virtual path, compatible with Leia and Matrix+."""
    try:
        return xbmcvfs.translatePath(path)
    except AttributeError:
        return xbmc.translatePath(path)


class PluginScanner(object):
    """Discovers VST plugins from the cache and/or filesystem."""

    def __init__(self, vst_directory, vsthost_data_path):
        """
        :param vst_directory:    User-configured directory to scan for VSTs.
        :param vsthost_data_path: Path to the audiodsp.vsthost data directory
                                  (contains plugin_cache.json).
        """
        # Translate any special:// paths to real filesystem paths
        self._vst_dir = _translate_path(vst_directory)
        self._cache_file = os.path.join(vsthost_data_path, 'plugin_cache.json')

    def get_all_plugins(self):
        """Return a list of plugin info dicts, merging cache and directory scan.

        Cached plugins (from vstscanner.exe) provide richer metadata (name,
        vendor, parameter count).  Directory-scanned plugins are added for any
        files not already in the cache.

        Only successfully scanned plugins are returned (those with no error).
        """
        cached = self._read_cache()
        scanned = self._scan_directory()

        if not cached:
            return scanned

        # Merge: start with cache, add uncached files from directory scan
        cached_paths = set(p['path'] for p in cached)
        merged = list(cached)
        for plugin in scanned:
            if plugin['path'] not in cached_paths:
                merged.append(plugin)

        return merged

    # ------------------------------------------------------------------
    # Cache reading
    # ------------------------------------------------------------------

    def _read_cache(self):
        """Read plugin_cache.json and return successfully scanned plugins."""
        if not xbmcvfs.exists(self._cache_file):
            return []
        try:
            f = xbmcvfs.File(self._cache_file, 'r')
            content = f.read()
            f.close()
            if not content:
                return []
            data = json.loads(content)
            # Filter out plugins that had scan errors
            return [p for p in data.get('plugins', [])
                    if not p.get('error')]
        except Exception:
            return []

    # ------------------------------------------------------------------
    # Directory scanning
    # ------------------------------------------------------------------

    def _scan_directory(self):
        """Walk the VST directory and return info dicts for each plugin file.

        This is a lightweight filesystem scan — it does NOT load the DLLs.
        File extension determines the assumed format:
          - ``.dll`` → VST2
          - ``.vst3`` → VST3
        """
        plugins = []
        if not self._vst_dir or not os.path.isdir(self._vst_dir):
            return plugins

        for root, _dirs, files in os.walk(self._vst_dir):
            for filename in files:
                filepath = os.path.join(root, filename)
                name, ext = os.path.splitext(filename)
                ext_lower = ext.lower()

                if ext_lower == '.dll':
                    plugins.append(self._make_entry(filepath, name, 'vst2'))
                elif ext_lower == '.vst3':
                    plugins.append(self._make_entry(filepath, name, 'vst3'))

        return plugins

    @staticmethod
    def _make_entry(filepath, name, fmt):
        """Create a minimal plugin info dict from a filesystem entry."""
        return {
            'path': filepath,
            'format': fmt,
            'name': name,
            'vendor': '',
            'numParams': 0,
        }
