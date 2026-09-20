# SPDX-License-Identifier: GPL-3.0-or-later

"""SQLite disk cache for artwork data between getdetails and getartwork."""

import json
import os
import sqlite3
import time

import xbmcvfs

from lib import log
from lib.config import ADDON

_db_path = ''
_initialized = False
_TTL = 86400


def _reset_initialized():
    """Force table re-creation on next _open() call."""
    global _initialized
    _initialized = False


def _open():
    """Open a connection, initializing the DB on first use."""
    global _db_path, _initialized

    if not _db_path:
        addon_data = xbmcvfs.translatePath(ADDON.getAddonInfo('profile'))
        if not os.path.exists(addon_data):
            os.makedirs(addon_data)
        _db_path = os.path.join(addon_data, 'art_cache.db')

    try:
        conn = sqlite3.connect(_db_path)
        if not _initialized:
            conn.execute(
                'CREATE TABLE IF NOT EXISTS art_cache '
                '(tmdb_id TEXT PRIMARY KEY, data TEXT)'
            )
            conn.execute(
                'CREATE TABLE IF NOT EXISTS art_meta '
                '(key TEXT PRIMARY KEY, value TEXT)'
            )
            conn.commit()
            _initialized = True
        return conn
    except sqlite3.Error as exc:
        log.error('art_cache open failed: {}'.format(exc))
        return None


def store(tmdb_id, show):
    """Write stripped artwork data for a show after fanart.tv merge."""
    stripped = {
        'name': show.get('name', ''),
        'images': show.get('images', {}),
        'seasons': [
            {
                'season_number': s.get('season_number', 0),
                'images': s.get('images', {}),
            }
            for s in show.get('seasons', []) if s.get('images')
        ],
    }
    conn = _open()
    if not conn:
        return
    try:
        conn.execute(
            'INSERT OR REPLACE INTO art_cache (tmdb_id, data) '
            'VALUES (?, ?)',
            (str(tmdb_id), json.dumps(stripped, separators=(',', ':'))),
        )
        conn.execute(
            "INSERT OR REPLACE INTO art_meta (key, value) "
            "VALUES ('updated', ?)",
            (str(int(time.time())),),
        )
        conn.commit()
    except sqlite3.Error as exc:
        _reset_initialized()
        log.error('art_cache store failed: {}'.format(exc))
    finally:
        conn.close()


def load(tmdb_id):
    """Read artwork data for a show. Returns stripped dict or None."""
    conn = _open()
    if not conn:
        return None
    try:
        row = conn.execute(
            'SELECT data FROM art_cache WHERE tmdb_id = ?',
            (str(tmdb_id),),
        ).fetchone()
        if row:
            return json.loads(row[0])
    except (sqlite3.Error, ValueError) as exc:
        _reset_initialized()
        log.error('art_cache load failed: {}'.format(exc))
    finally:
        conn.close()
    return None


def _wipe(conn, reason):
    """Delete all data and reclaim disk space."""
    deleted = conn.execute('DELETE FROM art_cache').rowcount
    conn.execute('DELETE FROM art_meta')
    conn.commit()
    conn.execute('VACUUM')
    if deleted:
        log.debug('art_cache: {}, cleared {} entries'.format(reason, deleted))
    return deleted


def check_and_clear():
    """Clear cache if data is stale or orphaned."""
    conn = _open()
    if not conn:
        return False
    try:
        row = conn.execute(
            "SELECT value FROM art_meta WHERE key='updated'"
        ).fetchone()
        if not row:
            return bool(_wipe(conn, 'orphaned data'))
        if time.time() - int(row[0]) > _TTL:
            _wipe(conn, 'TTL expired')
            return True
    except sqlite3.Error as exc:
        log.error('art_cache check_and_clear failed: {}'.format(exc))
    finally:
        conn.close()
    return False
