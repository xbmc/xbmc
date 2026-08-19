# SPDX-License-Identifier: GPL-3.0-or-later

"""TMDB API v3 client with batched requests and in-memory cache.

Uses append_to_response to minimize API calls. Module-level cache
survives between Kodi plugin calls via reuselanguageinvoker=true.
"""

import gzip
import json
from collections import OrderedDict
from urllib.request import Request, urlopen
from urllib.parse import urlencode

from lib import log
from lib.config import API_HEADERS, CACHE_LIMIT, TMDB_API_KEY

_BASE = 'https://api.themoviedb.org/3'
# season appends repeat the whole show payload
_HEADERS = dict(API_HEADERS, **{'Accept-Encoding': 'gzip'})
# TMDB rejects more than 20 appends
_MAX_APPENDS = 20
# soaps credit hundreds, aggregate fallback thousands
_MAX_CAST = 200
# {show_id: {'show': dict, 'episodes': {(s,e): dict}}}
_cache = OrderedDict()
_img_base = ''


def _read_json(resp):
    """Decode a response body, ungzipping it when TMDB compressed it."""
    raw = resp.read()
    if resp.headers.get('Content-Encoding') == 'gzip':
        raw = gzip.decompress(raw)
    return json.loads(raw.decode('utf-8'))


def get_image_base():
    """Image base URL from /configuration, fetched once per session."""
    global _img_base
    if not _img_base:
        url = '{}{}?{}'.format(_BASE, '/configuration',
                               urlencode({'api_key': TMDB_API_KEY}))
        try:
            req = Request(url, headers=_HEADERS)
            with urlopen(req, timeout=10) as resp:
                data = _read_json(resp)
            _img_base = data['images']['secure_base_url']
            log.debug('TMDB image base: {}'.format(_img_base))
        except Exception:
            _img_base = 'https://image.tmdb.org/t/p/'
    return _img_base


def _top_role(member):
    """Character the actor played most, which TMDB lists first."""
    roles = member.get('roles') or []
    return (roles[0].get('character') or '') if roles else ''


def _season_regulars(show):
    """Regulars across every season, deduped, in TMDB's billing order."""
    found = OrderedDict()
    for season in show.get('seasons', []):
        for member in season.get('credits', {}).get('cast', []):
            name = member.get('name', '')
            if not name:
                continue
            billing = member.get('order')
            billing = _MAX_CAST if billing is None else billing
            # billing is show-wide, so keep the season they ranked highest in
            if name not in found or billing < found[name][0]:
                found[name] = [billing, member]
    ranked = sorted(found.values(), key=lambda e: e[0])
    return [{
        'name': member.get('name', ''),
        'character': member.get('character', ''),
        'order': i,
        'profile_path': member.get('profile_path'),
    } for i, (_, member) in enumerate(ranked[:_MAX_CAST])]


class TmdbApi:
    """TMDB client for one scrape, holding the language preferences."""

    def __init__(self, settings):
        self._lang = settings['lang_details']
        img_lang = settings['lang_images'][:2]
        self._img_lang = '{},null'.format(img_lang) if img_lang == 'en' \
            else '{},en,null'.format(img_lang)
        self._is_english = self._lang.startswith('en')

    def search_shows(self, title, year=''):
        """Search results for a title, narrowed by first-air year if given."""
        params = {'query': title, 'language': self._lang}
        if year:
            params['first_air_date_year'] = year
        data = self._get('/search/tv', params)
        return data.get('results', []) if data else []

    def find_by_external_id(self, external_id, source):
        """IMDB/TVDB ID -> TMDB show ID via /find endpoint."""
        data = self._get('/find/{}'.format(external_id), {
            'external_source': '{}_id'.format(source),
        })
        if not data:
            return None
        results = data.get('tv_results', [])
        return str(results[0]['id']) if results else None

    def get_show_details(self, show_id):
        """Show metadata, per-season images and cast, batched and cached."""
        show_id = str(show_id)
        cached = _cache.get(show_id, {}).get('show')
        if cached:
            _cache.move_to_end(show_id)
            return cached

        # Evict oldest shows beyond limit
        while len(_cache) >= CACHE_LIMIT:
            evicted_id, _ = _cache.popitem(last=False)
            log.debug('cache evict show {}'.format(evicted_id))

        show = self._get('/tv/{}'.format(show_id), {
            'language': self._lang,
            'append_to_response': ','.join([
                'credits', 'content_ratings', 'external_ids',
                'images', 'videos', 'keywords',
            ]),
            'include_image_language': self._img_lang,
            'include_video_language': self._img_lang,
        })
        if not show:
            return None

        if not self._is_english and not show.get('overview'):
            en = self._get('/tv/{}'.format(show_id), {'language': 'en-US'})
            if en and en.get('overview'):
                show['overview'] = en['overview']

        self._attach_season_data(show)
        self._set_series_cast(show_id, show)
        _cache.setdefault(show_id, {})['show'] = show
        return show

    def _set_series_cast(self, show_id, show):
        """Show cast from the season regulars, or the whole run if none exist."""
        cast = _season_regulars(show)
        if not cast:
            cast = self._aggregate_cast(show_id)
        if cast:
            show.setdefault('credits', {})['cast'] = cast
            log.debug('series cast: {}'.format(len(cast)))

    def _aggregate_cast(self, show_id):
        """Fallback for shows credited per episode with no season regulars."""
        data = self._get('/tv/{}/aggregate_credits'.format(show_id), {
            'language': self._lang,
        })
        if not data:
            return []
        return [{
            'name': member.get('name', ''),
            'character': _top_role(member),
            'order': i,
            'profile_path': member.get('profile_path'),
        } for i, member in enumerate(data.get('cast', [])[:_MAX_CAST])]

    def _attach_season_data(self, show):
        """Batch-fetch and attach per-season images and regular cast."""
        seasons = show.get('seasons', [])
        season_map = {s.get('season_number', 0): s for s in seasons}
        show_id = show['id']

        season_keys = list(season_map.keys())
        per_call = _MAX_APPENDS // 2
        for i in range(0, len(season_keys), per_call):
            batch = season_keys[i:i + per_call]
            appends = []
            for n in batch:
                appends.append('season/{}/images'.format(n))
                appends.append('season/{}/credits'.format(n))
            data = self._get('/tv/{}'.format(show_id), {
                'language': self._lang,
                'append_to_response': ','.join(appends),
                'include_image_language': self._img_lang,
            })
            if not data:
                continue
            for snum in batch:
                images = data.get('season/{}/images'.format(snum))
                if images:
                    season_map[snum]['images'] = images
                credits = data.get('season/{}/credits'.format(snum))
                if credits:
                    season_map[snum]['credits'] = credits

    def prefetch_episodes(self, show_id):
        """Pre-fetch all episode data for the entire show."""
        show_id = str(show_id)
        entry = _cache.setdefault(show_id, {})
        if entry.get('episodes'):
            return

        show = entry.get('show')
        if not show:
            show = self.get_show_details(show_id)
        if not show:
            return

        season_nums = [
            s.get('season_number', 0) for s in show.get('seasons', [])
        ]

        all_seasons = self._fetch_all_seasons(show_id, season_nums)
        episodes = self._fetch_episode_extras(
            show_id, season_nums, all_seasons
        )

        if not self._is_english:
            self._episode_lang_fallback(show_id, episodes)

        entry['episodes'] = episodes

    def _fetch_all_seasons(self, show_id, season_nums):
        """Phase 1: Fetch full season data via show endpoint appends."""
        result = {}
        for i in range(0, len(season_nums), _MAX_APPENDS):
            batch = season_nums[i:i + _MAX_APPENDS]
            data = self._get('/tv/{}'.format(show_id), {
                'language': self._lang,
                'append_to_response': ','.join(
                    'season/{}'.format(n) for n in batch
                ),
            })
            if not data:
                continue
            for snum in batch:
                sd = data.get('season/{}'.format(snum))
                if sd:
                    result[snum] = sd
        return result

    def _fetch_episode_extras(self, show_id, season_nums, all_seasons):
        """Phase 2: Episode images and external_ids, two appends per episode."""
        episodes = {}
        per_call = _MAX_APPENDS // 2

        for snum in season_nums:
            sd = all_seasons.get(snum)
            if not sd:
                continue
            eps = sd.get('episodes', [])
            if not eps:
                continue

            eps = [e for e in eps if 'episode_number' in e]
            ep_nums = [e['episode_number'] for e in eps]
            ep_by_num = {e['episode_number']: e for e in eps}
            i = 0

            while i < len(ep_nums):
                batch = ep_nums[i:i + per_call]

                appends = []
                for en in batch:
                    appends.extend([
                        'episode/{}/images'.format(en),
                        'episode/{}/external_ids'.format(en),
                    ])

                data = self._get(
                    '/tv/{}/season/{}'.format(show_id, snum), {
                        'language': self._lang,
                        'append_to_response': ','.join(appends),
                        'include_image_language': self._img_lang,
                    }
                )

                if not data:
                    i += len(batch)
                    continue

                for en in batch:
                    base = ep_by_num.get(en)
                    if not base:
                        continue
                    ep = dict(base)
                    img = data.get('episode/{}/images'.format(en))
                    ext = data.get('episode/{}/external_ids'.format(en))
                    if img:
                        ep['images'] = img
                    if ext:
                        ep['external_ids'] = ext
                    episodes[(snum, en)] = ep

                i += len(batch)

        return episodes

    def _episode_lang_fallback(self, show_id, episodes):
        """Fill missing episode names/overviews from English."""
        need = set()
        for (snum, enum), ep in episodes.items():
            name = ep.get('name') or ''
            if (not name or not ep.get('overview')
                    or name == 'Episode {}'.format(enum)):
                need.add(snum)

        for snum in need:
            en = self._get(
                '/tv/{}/season/{}'.format(show_id, snum),
                {'language': 'en-US'}
            )
            if not en:
                continue
            for en_ep in en.get('episodes', []):
                enum = en_ep.get('episode_number', 0)
                ep = episodes.get((snum, enum))
                if not ep:
                    continue
                name = ep.get('name') or ''
                if not name or name == 'Episode {}'.format(enum):
                    if en_ep.get('name'):
                        ep['name'] = en_ep['name']
                if not ep.get('overview') and en_ep.get('overview'):
                    ep['overview'] = en_ep['overview']

    def get_episode(self, show_id, season_num, episode_num):
        """Cache-first episode read. Single API call on cache miss."""
        show_id = str(show_id)
        cached = _cache.get(show_id, {}).get('episodes', {}).get(
            (season_num, episode_num)
        )
        if cached:
            return cached

        data = self._get(
            '/tv/{}/season/{}/episode/{}'.format(
                show_id, season_num, episode_num
            ), {
                'language': self._lang,
                'append_to_response': 'images,external_ids',
                'include_image_language': self._img_lang,
            }
        )
        if data:
            entry = _cache.setdefault(show_id, {})
            entry.setdefault('episodes', {})[(season_num, episode_num)] = data
        return data

    def get_cached_episodes(self, show_id):
        """All cached episodes for iteration. Empty dict if not prefetched."""
        return _cache.get(str(show_id), {}).get('episodes', {})

    def get_episode_group(self, group_id):
        """Fetch alternate episode ordering."""
        return self._get('/tv/episode_group/{}'.format(group_id), {})

    def _get(self, path, params):
        """GET a TMDB endpoint, returning parsed JSON or None if it failed."""
        query = dict(params, api_key=TMDB_API_KEY)
        url = '{}{}?{}'.format(_BASE, path, urlencode(query))
        log.debug('TMDB GET {}'.format(path))
        try:
            req = Request(url, headers=_HEADERS)
            with urlopen(req, timeout=15) as resp:
                return _read_json(resp)
        except Exception as exc:
            from urllib.error import HTTPError
            if isinstance(exc, HTTPError) and exc.code == 404:
                log.info('TMDB GET {}: not found'.format(path))
            else:
                log.error('TMDB GET {} failed: {}'.format(path, exc))
            return None
