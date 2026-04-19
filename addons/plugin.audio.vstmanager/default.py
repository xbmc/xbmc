# -*- coding: utf-8 -*-
"""
default.py — VST Manager main entry point.

A Kodi plugin-source addon that lists VST plugins and lets the user add or
remove them from the audio processing chain managed by audiodsp.vsthost.

License: GPL-2.0-or-later
"""
from __future__ import unicode_literals

import os
import sys

try:
    from urllib import urlencode
    from urlparse import parse_qsl
except ImportError:  # Python 3
    from urllib.parse import urlencode, parse_qsl

import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon
import xbmcvfs


def _translate_path(path):
    """Translate a Kodi virtual path, compatible with Leia and Matrix+."""
    try:
        return xbmcvfs.translatePath(path)
    except AttributeError:
        return xbmc.translatePath(path)


# ---------------------------------------------------------------------------
# Globals
# ---------------------------------------------------------------------------

ADDON = xbmcaddon.Addon()
ADDON_ID = ADDON.getAddonInfo('id')
HANDLE = int(sys.argv[1])
BASE_URL = sys.argv[0]
ADDON_PATH = _translate_path(ADDON.getAddonInfo('path'))

# Insert our library path so imports work
sys.path.insert(0, os.path.join(ADDON_PATH, 'resources', 'lib'))

from chain_manager import ChainManager      # noqa: E402
from plugin_scanner import PluginScanner    # noqa: E402
from editor_bridge import open_editor as bridge_open_editor   # noqa: E402
from editor_bridge import close_editor as bridge_close_editor  # noqa: E402

# Shared data directory with the C++ audiodsp.vsthost ADSP addon
VSTHOST_DATA = _translate_path(
    'special://profile/addon_data/audiodsp.vsthost/')


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def log(msg, level=xbmc.LOGDEBUG):
    """Write a prefixed log message to the Kodi log."""
    xbmc.log('[%s] %s' % (ADDON_ID, msg), level)


def build_url(query):
    """Encode *query* dict into a full plugin URL."""
    return BASE_URL + '?' + urlencode(query)


def get_vst_directory():
    """Return the user-configured VST directory.

    The typed path (``vst_directory_text``) takes precedence over the browsed
    path (``vst_directory``) so users can choose whichever input method they
    prefer.
    """
    text_dir = ADDON.getSetting('vst_directory_text').strip()
    browse_dir = ADDON.getSetting('vst_directory').strip()
    return text_dir if text_dir else browse_dir


# ---------------------------------------------------------------------------
# Main listing
# ---------------------------------------------------------------------------

def list_vsts():
    """Build the directory listing of all discovered VST plugins."""
    vst_dir = get_vst_directory()
    if not vst_dir:
        xbmcgui.Dialog().ok(
            'VST Manager',
            'No VST directory configured.',
            'Please set the VST Plugin Directory in addon settings.')
        xbmcplugin.endOfDirectory(HANDLE, succeeded=False)
        return

    # Discover plugins from cache and/or directory scan
    scanner = PluginScanner(vst_dir, VSTHOST_DATA)
    plugins = scanner.get_all_plugins()

    if not plugins:
        xbmcgui.Dialog().ok(
            'VST Manager',
            'No VST plugins found in:',
            vst_dir)
        xbmcplugin.endOfDirectory(HANDLE, succeeded=False)
        return

    # Determine which plugins are already in the audio chain
    chain_mgr = ChainManager(VSTHOST_DATA)
    # Quietly remove chain entries whose files no longer exist
    removed = chain_mgr.cleanup_missing()
    if removed:
        log('Cleaned up %d missing plugin(s) from chain' % removed)

    chain_paths = chain_mgr.get_chain_paths()

    for plugin in plugins:
        path = plugin['path']
        in_chain = path in chain_paths
        name = plugin.get('name') or os.path.splitext(
            os.path.basename(path))[0]
        prefix = '- ' if in_chain else '+ '
        label = prefix + name

        li = xbmcgui.ListItem(label=label)
        li.setInfo('music', {'title': label})

        # Common URL parameters for this plugin
        params = {
            'path': path,
            'name': name,
            'vendor': plugin.get('vendor', ''),
            'format': plugin.get('format', 'vst2'),
            'numParams': str(plugin.get('numParams', 0)),
        }

        if in_chain:
            # Single click → show VST UI
            url = build_url(dict(params, action='show_ui'))
            # Right-click / long-press → context menu with remove and close options
            context_items = [
                ('Remove from audio chain',
                 'RunPlugin(%s)' % build_url(dict(params, action='remove'))),
                ('Close VST editor',
                 'RunPlugin(%s)' % build_url(dict(params, action='close_ui')))
            ]
            li.addContextMenuItems(context_items)
        else:
            # Single click → ask to add to chain
            url = build_url(dict(params, action='add'))

        xbmcplugin.addDirectoryItem(
            handle=HANDLE, url=url, listitem=li, isFolder=False)

    xbmcplugin.addSortMethod(HANDLE, xbmcplugin.SORT_METHOD_LABEL)
    xbmcplugin.endOfDirectory(HANDLE)


# ---------------------------------------------------------------------------
# Actions
# ---------------------------------------------------------------------------

def action_add(params):
    """Prompt the user, then add a VST plugin to the audio chain."""
    path = params.get('path', '')
    fmt = params.get('format', 'vst2')
    name = params.get('name') or os.path.splitext(
        os.path.basename(path))[0]

    confirmed = xbmcgui.Dialog().yesno(
        'VST Manager',
        'Add "%s" to the audio chain?' % name)

    if not confirmed:
        return

    chain_mgr = ChainManager(VSTHOST_DATA)
    try:
        chain_mgr.add_plugin(path, fmt)
        log('Added plugin to chain: %s' % path)
        # Refresh the listing so the prefix changes from + to -
        xbmc.executebuiltin('Container.Refresh')
        # Show the VST UI for the newly added plugin
        show_vst_ui(params)
    except Exception as exc:
        log('Failed to add plugin %s: %s' % (path, exc), xbmc.LOGERROR)
        # Remove the failed plugin from the chain to prevent corruption
        chain_mgr.remove_plugin(path)
        xbmcgui.Dialog().notification(
            'VST Manager',
            'Failed to load "%s"' % name,
            xbmcgui.NOTIFICATION_ERROR,
            3000)


def action_remove(params):
    """Prompt the user, then remove a VST plugin from the audio chain."""
    path = params.get('path', '')
    name = params.get('name') or os.path.splitext(
        os.path.basename(path))[0]

    confirmed = xbmcgui.Dialog().yesno(
        'VST Manager',
        'Remove "%s" from the audio chain?' % name)

    if not confirmed:
        return

    # Close any open native editor window first
    bridge_close_editor(path)

    chain_mgr = ChainManager(VSTHOST_DATA)
    chain_mgr.remove_plugin(path)
    log('Removed plugin from chain: %s' % path)
    xbmc.executebuiltin('Container.Refresh')


def show_vst_ui(params):
    """Open the native VST editor window via the C++ EditorBridge.

    Falls back to a metadata dialog if the bridge is unreachable or the
    plugin has no graphical editor.
    """
    path = params.get('path', '')
    name = params.get('name', 'Unknown')
    vendor = params.get('vendor', '')
    fmt = params.get('format', 'unknown')
    num_params = params.get('numParams', '0')

    # Try to open the native editor via the C++ bridge
    result = bridge_open_editor(path)

    if result and result.get('status') == 'ok':
        if result.get('hasEditor') is True:
            # Native editor opened successfully — the C++ side manages the
            # window (including the X close button).  Nothing more to do here.
            log('Opened native VST editor for: %s' % name)
            return
        # Plugin has no editor — fall through to metadata dialog

    # Fallback: show metadata dialog (bridge unreachable or no editor)
    heading = 'VST Plugin - %s' % name
    line1 = 'Vendor: %s' % (vendor if vendor else 'Unknown')
    line2 = 'Format: %s  |  Parameters: %s' % (fmt.upper(), num_params)
    line3 = 'Status: Active in audio chain'

    xbmcgui.Dialog().ok(heading, line1, line2, line3)


def close_vst_ui(params):
    """Close a native VST editor window via the C++ EditorBridge."""
    path = params.get('path', '')
    name = params.get('name', 'Unknown')

    result = bridge_close_editor(path)
    if result and result.get('status') == 'ok':
        log('Closed native VST editor for: %s' % name)
    else:
        log('Could not close editor for %s (bridge unreachable?)' % name)


# ---------------------------------------------------------------------------
# Router
# ---------------------------------------------------------------------------

def main():
    """Parse the query string and route to the appropriate handler."""
    params = dict(parse_qsl(sys.argv[2].lstrip('?')))
    action = params.get('action', '')

    if action == 'add':
        action_add(params)
    elif action == 'remove':
        action_remove(params)
    elif action == 'show_ui':
        show_vst_ui(params)
    elif action == 'close_ui':
        close_vst_ui(params)
    else:
        list_vsts()


if __name__ == '__main__':
    main()
