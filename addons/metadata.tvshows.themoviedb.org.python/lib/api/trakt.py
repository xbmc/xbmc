# SPDX-License-Identifier: GPL-3.0-or-later

"""Trakt API client for show and episode ratings."""

import json
from collections import OrderedDict
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from lib import log
from lib.config import API_HEADERS, CACHE_LIMIT, TRAKT_CLIENTID

_BASE = 'https://api.trakt.tv'

_HEADERS = dict(API_HEADERS, **{
    'trakt-api-version': '2',
    'trakt-api-key': TRAKT_CLIENTID,
})

# Per-episode rating cache, keyed by (imdb_id, season, episode)
# Survives between plugin calls via reuselanguageinvoker=true
_episode_cache = {}
_cached_shows = OrderedDict()


def get_show_rating(imdb_id):
    """Fetch Trakt rating for a show. Returns (rating, votes) or None."""
    data = _get('/shows/{}'.format(imdb_id), {'extended': 'full'})
    if not data:
        return None
    rating = data.get('rating')
    votes = data.get('votes')
    if rating is not None and votes:
        return float(rating), int(votes)
    return None


def prefetch_season_ratings(imdb_id, season):
    """Fetch all episode ratings for a season in one call, cache results."""
    data = _get('/shows/{}/seasons/{}'.format(imdb_id, season),
                {'extended': 'full'})
    if not data or not isinstance(data, list):
        return
    for ep in data:
        ep_num = ep.get('number')
        if ep_num is None:
            continue
        rating = ep.get('rating')
        votes = ep.get('votes')
        if rating is not None and votes:
            # Safety net if _cached_shows eviction is ever broken
            if len(_episode_cache) >= 50000:
                _episode_cache.clear()
            _episode_cache[(imdb_id, season, ep_num)] = (
                float(rating), int(votes)
            )


def prefetch_show_ratings(imdb_id, season_nums):
    """Prefetch Trakt episode ratings for all given seasons."""
    if imdb_id in _cached_shows:
        _cached_shows.move_to_end(imdb_id)
        return

    while len(_cached_shows) >= CACHE_LIMIT:
        evict_id, _ = _cached_shows.popitem(last=False)
        to_remove = [k for k in _episode_cache if k[0] == evict_id]
        for k in to_remove:
            del _episode_cache[k]

    _cached_shows[imdb_id] = True
    for s in season_nums:
        prefetch_season_ratings(imdb_id, s)


def get_episode_rating(imdb_id, season, episode):
    """Return cached Trakt episode rating, or fetch single if cache miss."""
    cached = _episode_cache.get((imdb_id, season, episode))
    if cached:
        return cached

    data = _get('/shows/{}/seasons/{}/episodes/{}/ratings'.format(
        imdb_id, season, episode
    ))
    if not data:
        return None
    rating = data.get('rating')
    votes = data.get('votes')
    if rating is not None and votes:
        result = (float(rating), int(votes))
        _episode_cache[(imdb_id, season, episode)] = result
        return result
    return None


def _get(path, params=None):
    url = '{}{}'.format(_BASE, path)
    if params:
        url = '{}?{}'.format(url, urlencode(params))
    log.debug('Trakt GET {}'.format(path))
    try:
        req = Request(url, headers=_HEADERS)
        with urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode('utf-8'))
    except Exception as exc:
        from urllib.error import HTTPError
        if isinstance(exc, HTTPError) and exc.code == 404:
            log.info('Trakt GET {}: not found'.format(path))
        else:
            log.error('Trakt GET {} failed: {}'.format(path, exc))
        return None
