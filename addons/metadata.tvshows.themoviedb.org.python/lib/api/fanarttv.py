# SPDX-License-Identifier: GPL-3.0-or-later

"""Fanart.tv client: fetch and merge banners, clearart, characterart, etc."""

import json
from urllib.request import Request, urlopen
from urllib.parse import quote, urlencode

from lib import log
from lib.config import API_HEADERS, FANARTTV_BASE, FANARTTV_KEY, FANARTTV_MAPPING


def merge_fanarttv_artwork(show_info, settings):
    """Merge Fanart.tv images into show_info's images dicts.

    Entries have type='fanarttv' so artwork.py uses the raw URL
    instead of prepending TMDB image paths.
    """
    if not settings.get('enable_fanarttv'):
        return
    if show_info.get('_fanarttv_merged'):
        return

    tvdb_id = show_info.get('external_ids', {}).get('tvdb_id')
    if not tvdb_id:
        return

    data = _fetch(tvdb_id, settings.get('fanarttv_clientkey', ''))
    if not data:
        return

    show_info['_fanarttv_merged'] = True
    user_lang = settings['lang_images'][:2].lower()
    seasons = show_info.get('seasons', [])
    season_map = {s.get('season_number', 0): s for s in seasons}

    for fanarttv_type, category in FANARTTV_MAPPING.items():
        items = data.get(fanarttv_type)
        if not items:
            continue

        is_season = category.startswith('season_')
        dict_key = category[7:] if is_season else category
        # TMDB uses 'logos' as dict key, our mapping says 'clearlogo'
        if dict_key == 'clearlogo':
            dict_key = 'logos'

        for item in items:
            lang = item.get('lang', '')
            if lang and lang != '00' and lang != user_lang and lang != 'en':
                continue

            try:
                likes = int(item.get('likes') or 0)
                width = int(item.get('width') or 0)
                height = int(item.get('height') or 0)
            except (ValueError, TypeError):
                likes = width = height = 0

            entry = {
                'file_path': _safe_url(item.get('url', '')),
                'iso_639_1': lang if lang and lang != '00' else None,
                'vote_average': 0,
                'vote_count': likes,
                'width': width,
                'height': height,
                'type': 'fanarttv',
                'hd': fanarttv_type == 'hdtvlogo',
            }

            if is_season:
                season_val = item.get('season', '')
                if season_val == '' or season_val is None or season_val == 'all':
                    snum = 0
                else:
                    try:
                        snum = int(season_val)
                    except (ValueError, TypeError):
                        continue
                season = season_map.get(snum)
                if season:
                    imgs = season.setdefault('images', {})
                    imgs.setdefault(dict_key, []).append(entry)
            else:
                imgs = show_info.setdefault('images', {})
                imgs.setdefault(dict_key, []).append(entry)


def _safe_url(url):
    """Encode spaces and special chars in URL path (some Fanart.tv URLs have them)."""
    parts = url.split('/', 3)
    if len(parts) == 4:
        parts[3] = quote(parts[3], safe='/')
    return '/'.join(parts)


def _fetch(tvdb_id, client_key):
    params = {'api_key': FANARTTV_KEY}
    if client_key:
        params['client_key'] = client_key
    url = '{}/tv/{}?{}'.format(FANARTTV_BASE, tvdb_id, urlencode(params))
    log.debug('Fanart.tv GET /tv/{}'.format(tvdb_id))
    try:
        req = Request(url, headers=API_HEADERS)
        with urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode('utf-8'))
    except Exception as exc:
        from urllib.error import HTTPError
        if isinstance(exc, HTTPError) and exc.code == 404:
            log.info('Fanart.tv GET /tv/{}: not found'.format(tvdb_id))
        else:
            log.error('Fanart.tv GET /tv/{} failed: {}'.format(tvdb_id, exc))
        return None
