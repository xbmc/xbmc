# SPDX-License-Identifier: GPL-3.0-or-later

"""IMDb ratings via official dataset (title.ratings.tsv.gz).

Downloads the ~7MB gzipped TSV from IMDb's dataset program, parses it
into a SQLite database for instant indexed lookups. Uses HTTP
Last-Modified header to avoid redundant downloads.
"""

import gzip
import io
import os
import sqlite3
import time
from urllib.request import Request, urlopen

import xbmcvfs

from lib import log
from lib.config import ADDON, API_HEADERS

_DATASET_URL = 'https://datasets.imdbws.com/title.ratings.tsv.gz'
_BATCH_SIZE = 50000

_CHECK_INTERVAL = 6 * 3600

_db_path = ''
_conn = None
_last_check = 0.0


def get_rating(imdb_id):
    """Lookup IMDb rating. Returns (rating, votes) or None."""
    if not imdb_id:
        return None
    _ensure_db()
    if not _conn:
        return None
    try:
        row = _conn.execute(
            'SELECT rating, votes FROM ratings WHERE tconst = ?',
            (imdb_id,)
        ).fetchone()
        if row:
            return float(row[0]), int(row[1])
    except sqlite3.Error:
        pass
    return None


def check_update():
    """HEAD-check remote dataset and download if newer. Skips if checked recently."""
    global _conn, _last_check
    now = time.time()
    if (now - _last_check) < _CHECK_INTERVAL:
        return
    _last_check = now

    _init_path()

    remote_mod = _head_last_modified()
    if not remote_mod:
        return

    local_mod = _get_local_last_modified()
    if local_mod == remote_mod:
        log.debug('IMDb dataset: up to date ({})'.format(local_mod))
        return

    log.info('IMDb dataset: update available (local={}, remote={})'.format(
        local_mod or 'none', remote_mod
    ))

    if _conn:
        _conn.close()
        _conn = None

    _download_and_import(remote_mod)


def _ensure_db():
    """Open DB connection if not already open."""
    global _conn
    _init_path()

    if _conn:
        return

    if os.path.exists(_db_path):
        try:
            _conn = sqlite3.connect(_db_path)
        except sqlite3.Error as exc:
            log.error('IMDb DB connect failed: {}'.format(exc))


def _init_path():
    """Set _db_path once."""
    global _db_path
    if _db_path:
        return
    addon_data = xbmcvfs.translatePath(ADDON.getAddonInfo('profile'))
    if not os.path.exists(addon_data):
        os.makedirs(addon_data)
    _db_path = os.path.join(addon_data, 'imdb_ratings.db')


def _head_last_modified():
    """HEAD request to get Last-Modified header. Returns string or None."""
    try:
        req = Request(_DATASET_URL, method='HEAD', headers={
            'User-Agent': API_HEADERS['User-Agent'],
        })
        with urlopen(req, timeout=10) as resp:
            return resp.getheader('Last-Modified')
    except Exception as exc:
        log.error('IMDb HEAD check failed: {}'.format(exc))
        return None


def _get_local_last_modified():
    """Read stored Last-Modified from DB meta table."""
    if not _db_path or not os.path.exists(_db_path):
        return None
    if _conn:
        try:
            row = _conn.execute(
                "SELECT value FROM meta WHERE key = 'last_modified'"
            ).fetchone()
            return row[0] if row else None
        except sqlite3.Error:
            return None
    conn = None
    try:
        conn = sqlite3.connect(_db_path)
        row = conn.execute(
            "SELECT value FROM meta WHERE key = 'last_modified'"
        ).fetchone()
        return row[0] if row else None
    except sqlite3.Error:
        return None
    finally:
        if conn:
            conn.close()


def _download_and_import(last_modified):
    """Download dataset and rebuild SQLite DB. Writes to temp file first."""
    log.info('IMDb dataset: downloading title.ratings.tsv.gz')
    data = _download()
    if not data:
        return

    tmp_path = _db_path + '.tmp'
    log.info('IMDb dataset: parsing into SQLite')
    conn = None
    try:
        conn = sqlite3.connect(tmp_path)
        conn.execute(
            'CREATE TABLE ratings '
            '(tconst TEXT PRIMARY KEY, rating REAL, votes INTEGER)'
        )
        conn.execute(
            'CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT)'
        )

        batch = []
        with gzip.GzipFile(fileobj=io.BytesIO(data)) as gz:
            for raw_line in gz:
                line = raw_line.decode('utf-8').rstrip('\n')
                if line.startswith('tconst'):
                    continue
                parts = line.split('\t')
                if len(parts) < 3:
                    continue
                try:
                    batch.append((parts[0], float(parts[1]), int(parts[2])))
                except (ValueError, IndexError):
                    continue
                if len(batch) >= _BATCH_SIZE:
                    conn.executemany(
                        'INSERT INTO ratings VALUES (?, ?, ?)', batch
                    )
                    batch.clear()
        if batch:
            conn.executemany('INSERT INTO ratings VALUES (?, ?, ?)', batch)

        conn.execute(
            "INSERT INTO meta VALUES ('last_modified', ?)",
            (last_modified,)
        )
        conn.commit()
        conn.close()
        conn = None

        os.replace(tmp_path, _db_path)
        log.info('IMDb dataset: done')
    except (sqlite3.Error, ValueError, OSError) as exc:
        log.error('IMDb dataset import failed: {}'.format(exc))
        if conn:
            conn.close()
        if os.path.exists(tmp_path):
            try:
                os.remove(tmp_path)
            except OSError:
                pass


def _download():
    """Download the gzipped TSV. Returns bytes or None."""
    try:
        req = Request(_DATASET_URL, headers={
            'User-Agent': API_HEADERS['User-Agent'],
            'Accept-Encoding': 'identity',
        })
        with urlopen(req, timeout=30) as resp:
            return resp.read()
    except Exception as exc:
        log.error('IMDb dataset download failed: {}'.format(exc))
        return None
