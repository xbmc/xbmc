# SPDX-License-Identifier: GPL-3.0-or-later

"""Scraper entry points called by Kodi."""

import json
import re
import time

import xbmc
import xbmcgui
import xbmcplugin

from lib import art_cache, log
from lib.artwork import set_artwork
from lib.api import trakt
from lib.api.fanarttv import merge_fanarttv_artwork
from lib.api.tmdb import TmdbApi, get_image_base, _cache
from lib.api.imdb import get_rating as imdb_rating, check_update as imdb_check
from lib.api.trakt import get_show_rating as trakt_show, \
    get_episode_rating as trakt_episode, \
    prefetch_show_ratings as trakt_prefetch
from lib.config import get_settings

_NFO_TMDB = re.compile(
    r'(?:themoviedb\.org|tmdb\.org)(?:/\w{2})?/tv/(\d+)[^/]*'
    r'(?:/episode_groups?/([a-f0-9]+))?'
)
_NFO_TVDB = re.compile(
    r'thetvdb\.com(?:'
    r'/?\?.*?(?:tab=series&|)id=(\d+)'
    r'|/series/(\d+)'
    r'|/api/.+?/series/(\d+)'
    r'|.*?"id":\s*(\d+)'
    r')'
)
_NFO_IMDB = re.compile(r'imdb\.com/title/(tt\d+)')
_NFO_NAMED_SEASON = re.compile(
    r'<namedseason\s+number="(\d+)">([^<]+)</namedseason>'
)

_RE_PARAGRAPH = re.compile(r'</p>\s*<p>')
_RE_HTML_TAG = re.compile(r'<[^>]+>')

_SEARCH_IMDB = re.compile(r'^(tt\d+)$|^imdb/(tt\d+)$', re.IGNORECASE)
_SEARCH_TMDB = re.compile(r'^tmdb/(\d+)$', re.IGNORECASE)
_SEARCH_TVDB = re.compile(r'^tvdb/(\d+)$', re.IGNORECASE)

# Trailing parenthetical: "(US)", "(UK)", "(2023)", or broken "(US" from Kodi
_RE_PAREN_SUFFIX = re.compile(r'\s+\(([^)]+)\)?$')

_FIND_SOURCES = ['imdb', 'tvdb']


_active_show = ''

_IDLE_TTL = 15 * 60
_last_activity = 0


def _clear_in_memory_caches():
    _cache.clear()
    trakt._cached_shows.clear()
    trakt._episode_cache.clear()


def run_action(handle, action, params):
    """Dispatch a Kodi scraper action."""
    global _last_activity
    now = time.time()
    if _last_activity and now - _last_activity > _IDLE_TTL:
        log.debug('idle > {}s, wiping in-memory caches'.format(_IDLE_TTL))
        _clear_in_memory_caches()
    _last_activity = now

    settings = get_settings()
    log.init(settings.get('verbose_log', False))

    if action != 'getartwork':
        art_cache.check_and_clear()

    if action == 'NfoUrl':
        _nfo_url(handle, params)
        return

    api = TmdbApi(settings)

    if settings.get('default_rating') == 'IMDb' or settings.get('imdb_anyway'):
        imdb_check()

    actions = {
        'find': _find,
        'getdetails': _getdetails,
        'getepisodelist': _getepisodelist,
        'getepisodedetails': _getepisodedetails,
        'getartwork': _getartwork,
    }
    func = actions.get(action)
    if func:
        log.debug('action={} cache={}'.format(action, len(_cache)))
        func(handle, api, params, settings)
    else:
        log.info('unknown action: {}'.format(action))
        xbmcplugin.endOfDirectory(handle)


def _evict_previous_show(cache, new_show_id):
    """Evict the previous show when moving to a new one."""
    global _active_show
    if _active_show and _active_show != str(new_show_id):
        if cache.pop(_active_show, None):
            log.debug('cache evict show {}'.format(_active_show))
    _active_show = str(new_show_id)


def _find(handle, api, params, _settings):
    title = params.get('title', '')
    year = params.get('year', '')

    # TMDB search returns garbage with "(US)" etc. in the query
    country_hint = ''
    clean_title = title
    paren = _RE_PAREN_SUFFIX.search(title)
    if paren:
        inner = paren.group(1)
        clean_title = title[:paren.start()].strip()
        if len(inner) == 2 and inner.isalpha():
            country_hint = inner.upper()
        elif len(inner) == 4 and inner.isdigit() and not year:
            year = inner

    # Search by external ID if title looks like one
    results = _search_by_external_id(api, clean_title)
    if results is None:
        results = api.search_shows(clean_title, year)
        # Year might be off-by-one in folder name, retry unfiltered
        if not results and year:
            results = api.search_shows(clean_title)

    # Sort by title similarity + year proximity so Kodi auto-selects best match
    query_year = int(year) if year and year.isdigit() else 0
    query_lower = clean_title.lower()
    for show in results:
        show['_relevance'] = _search_relevance(
            show.get('name', ''), show.get('original_name', ''),
            show.get('first_air_date', ''), query_lower, query_year,
            show.get('origin_country', []), country_hint,
        )
    results.sort(key=lambda s: s['_relevance'], reverse=True)
    img_base = get_image_base()

    for show in results:
        name = show.get('name', '')
        show_id = show.get('id')
        if not name or not show_id:
            continue
        # Disambiguate: "Show Name (2023, US)"
        label = name
        parts = []
        aired = show.get('first_air_date', '')
        if aired and len(aired) >= 4:
            parts.append(aired[:4])
        origins = show.get('origin_country', [])
        if origins:
            parts.append(origins[0])
        if parts:
            label = '{} ({})'.format(name, ', '.join(parts))
        li = xbmcgui.ListItem(label, offscreen=True)
        vtag = li.getVideoInfoTag()
        vtag.setTitle(name)
        vtag.setOriginalTitle(show.get('original_name', ''))
        vtag.setUniqueIDs({'tmdb': str(show_id)}, 'tmdb')
        vtag.setPremiered(show.get('first_air_date', ''))
        vtag.setPlot(show.get('overview', ''))
        vtag.setMediaType('tvshow')
        poster = show.get('poster_path')
        if poster:
            vtag.addAvailableArtwork(
                '{}original{}'.format(img_base, poster),
                arttype='poster',
                preview='{}w500{}'.format(img_base, poster),
            )
        li.setProperty('relevance', str(show['_relevance']))
        xbmcplugin.addDirectoryItem(
            handle=handle, url=str(show_id),
            listitem=li, isFolder=True,
        )
    xbmcplugin.endOfDirectory(handle)


def _getdetails(handle, api, params, settings):
    show_id, ep_grouping, named_seasons = _resolve_show_id(api, params)
    if not show_id:
        _fail(handle)
        return

    _evict_previous_show(_cache, show_id)

    show = api.get_show_details(show_id)
    if not show:
        _fail(handle)
        return

    ep_group_data = None
    if ep_grouping:
        group = api.get_episode_group(ep_grouping)
        if group:
            ep_group_data = group.get('groups', [])
            log.debug('episode group: {} parts'.format(len(ep_group_data)))
            _add_season_stubs(show, ep_group_data)

    merge_fanarttv_artwork(show, settings)
    art_cache.store(show_id, show)

    li = xbmcgui.ListItem(show.get('name', ''), offscreen=True)
    _populate_show(li, show, settings, ep_grouping, named_seasons,
                   ep_group_data)
    set_artwork(li, show, settings)
    xbmcplugin.setResolvedUrl(handle, True, li)


def _getepisodelist(handle, api, params, settings):
    show_id, ep_grouping = _resolve_episode_guide(api, params)
    if not show_id:
        xbmcplugin.endOfDirectory(handle)
        return

    api.prefetch_episodes(show_id)

    episodes = api.get_cached_episodes(show_id)
    if not episodes:
        xbmcplugin.endOfDirectory(handle)
        return

    # Prefetch Trakt ratings per-season instead of per-episode
    default = settings.get('default_rating', 'TMDb')
    if default == 'Trakt' or settings.get('trakt_anyway'):
        show = api.get_show_details(show_id)
        show_imdb = ''
        if show:
            show_imdb = show.get('external_ids', {}).get('imdb_id', '')
        if show_imdb:
            season_nums = set(s for s, _ in episodes.keys())
            trakt_prefetch(show_imdb, season_nums)

    if ep_grouping:
        before = len(episodes)
        episodes = _apply_episode_grouping(api, ep_grouping, episodes)
        seasons = sorted(set(s for s, _ in episodes.keys()))
        log.debug('episode group: {} -> {} episodes, seasons {}'.format(
            before, len(episodes), seasons))

    for (snum, enum) in sorted(episodes.keys()):
        ep = episodes[(snum, enum)]
        li = xbmcgui.ListItem(ep.get('name', ''), offscreen=True)
        vtag = li.getVideoInfoTag()
        vtag.setSeason(snum)
        vtag.setEpisode(enum)
        vtag.setTitle(ep.get('name') or 'Episode {}'.format(enum))
        vtag.setFirstAired(ep.get('air_date', ''))
        vtag.setMediaType('episode')

        # URL uses original numbers so getepisodedetails finds cached data
        org_s = ep.get('org_seasonnum', snum)
        org_e = ep.get('org_epnum', enum)
        ep_url = '{}/{}/{}'.format(show_id, org_s, org_e)
        xbmcplugin.addDirectoryItem(
            handle=handle, url=ep_url,
            listitem=li, isFolder=False,
        )
    xbmcplugin.endOfDirectory(handle)


def _getepisodedetails(handle, api, params, settings):
    url = params.get('url', '')
    parts = url.split('/')
    if len(parts) < 3:
        _fail(handle)
        return
    show_id = parts[0]
    try:
        season_num = int(parts[1])
        episode_num = int(parts[2])
    except (ValueError, IndexError):
        _fail(handle)
        return

    ep = api.get_episode(show_id, season_num, episode_num)
    if not ep:
        _fail(handle)
        return

    # Show's IMDB ID needed for Trakt episode ratings
    show = api.get_show_details(show_id)
    show_imdb_id = ''
    if show:
        show_imdb_id = show.get('external_ids', {}).get('imdb_id', '')

    li = xbmcgui.ListItem(ep.get('name', ''), offscreen=True)
    _populate_episode(
        li, ep, season_num, episode_num, settings, show_imdb_id
    )
    xbmcplugin.setResolvedUrl(handle, True, li)


def _getartwork(handle, api, params, settings):
    show_id = params.get('id')
    if not show_id:
        show_id, _, _ = _resolve_show_id(api, params)
    if not show_id:
        _fail(handle)
        return

    show = _cache.get(str(show_id), {}).get('show')
    if show:
        log.debug('getartwork {}: memory hit'.format(show_id))
    else:
        show = art_cache.load(show_id)
        if show:
            log.debug('getartwork {}: sqlite hit'.format(show_id))
        else:
            log.debug('getartwork {}: api fetch'.format(show_id))
            show = api.get_show_details(show_id)
            if not show:
                _fail(handle)
                return
            merge_fanarttv_artwork(show, settings)

    li = xbmcgui.ListItem(show.get('name', ''), offscreen=True)
    set_artwork(li, show, settings)
    xbmcplugin.setResolvedUrl(handle, True, li)


def _nfo_url(handle, params):
    nfo = params.get('nfo', '')

    # Kodi handles complete NFOs (with <uniqueid>) itself, skip
    if '<uniqueid' in nfo:
        xbmcplugin.endOfDirectory(handle)
        return

    show_id, provider, ep_grouping, named_seasons = _parse_nfo(nfo)
    log.debug('NfoUrl: id={}, provider={}, ep_group={}'.format(
        show_id, provider, ep_grouping or 'none'))

    if not show_id:
        xbmcplugin.endOfDirectory(handle)
        return

    # IMDB/TVDB URLs need conversion to TMDB ID via /find
    if provider in ('imdb', 'tvdb'):
        settings = get_settings()
        api = TmdbApi(settings)
        tmdb_id = api.find_by_external_id(show_id, provider)
        if not tmdb_id:
            xbmcplugin.endOfDirectory(handle)
            return
        show_id = tmdb_id

    url = show_id
    if ep_grouping:
        url = '{}|{}'.format(show_id, ep_grouping)
    if named_seasons:
        ns = json.dumps({str(k): v for k, v in named_seasons.items()})
        url = '{}|ns:{}'.format(url, ns)

    li = xbmcgui.ListItem(offscreen=True)
    li.getVideoInfoTag().setUniqueIDs({'tmdb': show_id}, 'tmdb')
    xbmcplugin.addDirectoryItem(
        handle=handle, url=url, listitem=li, isFolder=True,
    )
    xbmcplugin.endOfDirectory(handle)


_RE_PUNCT = re.compile(r'[:\-,.!?…\u2013\u2014\']+')


def _normalize(text):
    return ' '.join(_RE_PUNCT.sub(' ', text).split())


def _title_relevance(query, title):
    """0.0-1.0: exact=1.0, prefix=0.95, contained=0.8, all words=0.6."""
    q = query.lower()
    t = title.lower()
    if not q or not t:
        return 0.0
    score = _title_match(q, t)
    if score >= 0.6:
        return score
    return _title_match(_normalize(q), _normalize(t))


def _title_match(q, t):
    if q == t:
        return 1.0
    # Query is a prefix up to a word boundary (colon, dash, space)
    if t.startswith(q) and (len(t) == len(q) or t[len(q)] in ':- '):
        return 0.95
    if q in t:
        return 0.8
    # All query words present in title
    qwords = q.split()
    if qwords and all(w in t for w in qwords):
        return 0.6
    return 0.0


def _search_relevance(name, original_name, first_air_date, query_lower,
                      query_year, origin_country=None, country_hint=''):
    """Score title + year + country match. Range roughly -0.6 to 3.0."""
    title_score = _title_relevance(query_lower, name)
    if original_name:
        alt = _title_relevance(query_lower, original_name)
        if alt > title_score:
            title_score = alt
    has_date = first_air_date and len(first_air_date) >= 4
    year_score = 0.0
    if query_year and has_date:
        try:
            result_year = int(first_air_date[:4])
            year_score = max(0.0, 1.0 - 0.5 * abs(query_year - result_year))
        except ValueError:
            pass
    # Stub entries with no air date rank below complete entries
    if not has_date:
        year_score = -0.1
    country_score = 0.0
    if country_hint and origin_country:
        country_score = 1.0 if country_hint in origin_country else -0.5
    return round(title_score + year_score + country_score, 4)


def _search_by_external_id(api, title):
    """Detect external ID patterns in search title. Returns list or None."""
    title = title.strip()

    match = _SEARCH_TMDB.match(title)
    if match:
        show = api.get_show_details(match.group(1))
        return [show] if show else []

    match = _SEARCH_IMDB.match(title)
    if match:
        imdb_id = match.group(1) or match.group(2)
        tmdb_id = api.find_by_external_id(imdb_id, 'imdb')
        if tmdb_id:
            show = api.get_show_details(tmdb_id)
            return [show] if show else []
        return []

    match = _SEARCH_TVDB.match(title)
    if match:
        tmdb_id = api.find_by_external_id(match.group(1), 'tvdb')
        if tmdb_id:
            show = api.get_show_details(tmdb_id)
            return [show] if show else []
        return []

    return None


def _resolve_show_id(api, params):
    """Extract (show_id, ep_grouping, named_seasons) from params."""
    url = params.get('url', '')
    ep_group = ''
    named = {}
    if url:
        parts = url.split('|')
        for part in parts[1:]:
            if part.startswith('ns:'):
                try:
                    raw = json.loads(part[3:])
                    named = {int(k): v for k, v in raw.items()}
                except (ValueError, TypeError):
                    pass
            elif not ep_group:
                ep_group = part

    uid_str = params.get('uniqueIDs', '')
    if uid_str:
        try:
            ids = json.loads(uid_str)
            tmdb_id = ids.get('tmdb', '')
            if tmdb_id:
                return str(tmdb_id), ep_group, named
            for source in _FIND_SOURCES:
                ext_id = ids.get(source, '')
                if ext_id:
                    found = api.find_by_external_id(ext_id, source)
                    if found:
                        return found, ep_group, named
        except (ValueError, TypeError):
            pass
    if not url:
        return '', '', {}

    base = url.split('|')[0]
    if base.isdigit():
        return base, ep_group, named
    return '', '', {}


def _resolve_episode_guide(api, params):
    """Extract (show_id, ep_grouping) from episode guide JSON or URL."""
    url = params.get('url', '')
    if not url:
        return '', ''

    try:
        ids = json.loads(url)
    except (ValueError, TypeError):
        ids = None
    # numeric urls parse as int, not dict
    if isinstance(ids, dict):
        tmdb_val = ids.get('tmdb', '')
        show_id = str(tmdb_val) if tmdb_val else ''
        if not show_id:
            for source in ('imdb', 'tvdb'):
                ext_id = ids.get(source, '')
                if ext_id:
                    found = api.find_by_external_id(str(ext_id), source)
                    if found:
                        show_id = found
                        break
        ep_group = ids.get('ep_group', '')
        return show_id, ep_group

    parts = url.split('|')
    base = parts[0]
    if base.isdigit():
        log.info('deprecated episodeguide format for show id {}, '
                 'refresh the show to update'.format(base))
        ep_group = parts[1] if len(parts) > 1 else ''
        return base, ep_group
    return '', ''


def _parse_nfo(nfo):
    """Parse NFO for show ID, provider, episode grouping, and named seasons."""
    named_seasons = {}
    for match in _NFO_NAMED_SEASON.finditer(nfo):
        named_seasons[int(match.group(1))] = match.group(2)

    match = _NFO_TMDB.search(nfo)
    if match:
        return match.group(1), 'tmdb', match.group(2) or '', named_seasons

    match = _NFO_TVDB.search(nfo)
    if match:
        # Regex has multiple capture groups for different URL formats
        tvdb_id = match.group(1) or match.group(2) or \
            match.group(3) or match.group(4)
        return tvdb_id, 'tvdb', '', named_seasons

    match = _NFO_IMDB.search(nfo)
    if match:
        return match.group(1), 'imdb', '', named_seasons

    return '', '', '', named_seasons


def _apply_episode_grouping(api, group_id, episodes):
    """Remap episode numbers per episode group ordering."""
    group = api.get_episode_group(group_id)
    if not group:
        return episodes

    remapped = {}
    missing = 0
    for grp in group.get('groups', []):
        new_season = grp.get('order', 0)
        for i, ep_entry in enumerate(grp.get('episodes', [])):
            orig_s = ep_entry.get('season_number')
            orig_e = ep_entry.get('episode_number')
            new_ep = ep_entry.get('order', i) + 1

            cached = episodes.get((orig_s, orig_e))
            if not cached:
                missing += 1
                continue

            ep = dict(cached)
            ep['org_seasonnum'] = orig_s
            ep['org_epnum'] = orig_e
            remapped[(new_season, new_ep)] = ep

    if missing:
        log.debug('episode group: {} episodes not in cache'.format(missing))
    return remapped if remapped else episodes


def _add_season_stubs(show, ep_group_data):
    """Add empty season entries for episode group parts missing from TMDB."""
    existing = {s.get('season_number', 0) for s in show.get('seasons', [])}
    added = 0
    for grp in ep_group_data:
        snum = grp.get('order', 0)
        if snum not in existing:
            show.setdefault('seasons', []).append({'season_number': snum})
            existing.add(snum)
            added += 1
    if added:
        log.debug('episode group: added {} season stubs'.format(added))


def _make_actor(member):
    thumb = ''
    if member.get('profile_path'):
        thumb = '{}original{}'.format(get_image_base(), member['profile_path'])
    return xbmc.Actor(
        member.get('name', ''), member.get('character', ''),
        member.get('order', 0), thumb,
    )


def _clean_plot(text):
    """Convert HTML formatting to Kodi tags, strip remaining HTML."""
    if not text:
        return ''
    text = text.replace('<b>', '[B]').replace('</b>', '[/B]')
    text = text.replace('<i>', '[I]').replace('</i>', '[/I]')
    text = _RE_PARAGRAPH.sub('[CR]', text)
    text = _RE_HTML_TAG.sub('', text)
    return text.strip()


def _fail(handle):
    xbmcplugin.setResolvedUrl(
        handle, False, xbmcgui.ListItem(offscreen=True)
    )


def _populate_show(li, show, settings, ep_grouping='', named_seasons=None,
                   ep_group_data=None):
    vtag = li.getVideoInfoTag()
    name = show.get('name', '')
    original = show.get('original_name', '')
    if settings.get('keep_original_title') and original:
        name = original

    vtag.setTitle(name)
    vtag.setOriginalTitle(original)
    vtag.setTvShowTitle(name)

    plot = _clean_plot(show.get('overview', ''))
    vtag.setPlot(plot)
    vtag.setPlotOutline(plot)
    vtag.setTagLine(show.get('tagline', ''))

    premiered = show.get('first_air_date', '')
    vtag.setPremiered(premiered)
    if premiered and len(premiered) >= 4 and premiered[:4].isdigit():
        vtag.setYear(int(premiered[:4]))

    vtag.setTvShowStatus(show.get('status', ''))
    vtag.setMediaType('tvshow')

    spoken = show.get('spoken_languages', [])
    orig_lang = spoken[0].get('iso_639_1', '') if spoken \
        else show.get('original_language', '')
    try:
        vtag.setOriginalLanguage(orig_lang)
    except AttributeError:
        pass

    vtag.setGenres([g.get('name', '') for g in show.get('genres', []) if g.get('name')])

    origins = show.get('origin_country', [])
    vtag.setCountries(origins)

    networks = show.get('networks', [])
    if networks:
        studio = networks[0].get('name', '')
        if studio:
            if settings.get('studio_country') and origins:
                studio = '{} ({})'.format(studio, origins[0])
            vtag.setStudios([studio])

    # IDs
    ids = {'tmdb': str(show.get('id', ''))}
    ext = show.get('external_ids', {})
    if ext.get('imdb_id'):
        ids['imdb'] = ext['imdb_id']
    if ext.get('tvdb_id'):
        ids['tvdb'] = str(ext['tvdb_id'])
    vtag.setUniqueIDs(ids, 'tmdb')

    guide = dict(ids)
    if ep_grouping:
        guide['ep_group'] = ep_grouping
    vtag.setEpisodeGuide(json.dumps(guide))

    # Ratings
    default = settings.get('default_rating', 'TMDb')
    imdb_id = ext.get('imdb_id', '')

    vote_avg = show.get('vote_average')
    if vote_avg and (default == 'TMDb' or settings.get('tmdb_anyway')):
        vtag.setRating(
            float(vote_avg), int(show.get('vote_count') or 0),
            'tmdb', default == 'TMDb',
        )

    if imdb_id and (default == 'Trakt' or settings.get('trakt_anyway')):
        trakt = trakt_show(imdb_id)
        if trakt:
            vtag.setRating(trakt[0], trakt[1], 'trakt', default == 'Trakt')

    if imdb_id and (default == 'IMDb' or settings.get('imdb_anyway')):
        imdb = imdb_rating(imdb_id)
        if imdb:
            vtag.setRating(imdb[0], imdb[1], 'imdb', default == 'IMDb')

    # Certification with US fallback
    cert_country = settings.get('cert_country', 'us').upper()
    mpaa = ''
    mpaa_us = ''
    for entry in show.get('content_ratings', {}).get('results', []):
        iso = entry.get('iso_3166_1', '').upper()
        if iso == cert_country:
            mpaa = entry.get('rating', '')
        if iso == 'US':
            mpaa_us = entry.get('rating', '')
    if not mpaa:
        mpaa = mpaa_us
    if mpaa:
        prefix = settings.get('cert_prefix', '') if settings.get('use_cert_prefix') else ''
        vtag.setMpaa('{}{}'.format(prefix, mpaa))

    # Cast
    credits = show.get('credits', {})
    actors = [_make_actor(m) for m in credits.get('cast', []) if m.get('name')]
    if actors:
        vtag.setCast(actors)

    # Writers (created_by + crew writers, deduplicated)
    writers = []
    seen = set()
    for creator in show.get('created_by', []):
        n = creator.get('name', '')
        if n and n not in seen:
            writers.append(n)
            seen.add(n)
    for member in credits.get('crew', []):
        n = member.get('name', '')
        job = member.get('job', '')
        dept = member.get('department', '')
        if (dept == 'Writing' or job == 'Writer') and n and n not in seen:
            writers.append(n)
            seen.add(n)
    if writers:
        vtag.setWriters(writers)

    # Seasons (with overview)
    named = named_seasons or {}
    season_source = ep_group_data or show.get('seasons', [])
    for season in season_source:
        if ep_group_data:
            snum = season.get('order', 0)
            sname = named.get(snum, season.get('name', ''))
            splot = ''
        else:
            snum = season.get('season_number', 0)
            sname = named.get(snum, season.get('name', ''))
            splot = _clean_plot(season.get('overview', ''))
        try:
            vtag.addSeason(snum, sname, splot)
        except TypeError:
            vtag.addSeason(snum, sname)

    # Runtime
    runtimes = show.get('episode_run_time', [])
    if runtimes:
        vtag.setDuration(runtimes[0] * 60)

    # Tags
    if settings.get('keywords_as_tags'):
        kw = show.get('keywords', {}).get('results', [])
        vtag.setTags([k.get('name', '') for k in kw if k.get('name')])

    # Trailer
    if settings.get('enable_trailer'):
        _set_trailer(vtag, show, settings)


def _populate_episode(li, ep, season_num, episode_num,
                      settings=None, show_imdb_id=''):
    vtag = li.getVideoInfoTag()

    title = ep.get('name') or 'Episode {}'.format(episode_num)
    vtag.setTitle(title)

    plot = _clean_plot(ep.get('overview', ''))
    vtag.setPlot(plot)
    vtag.setPlotOutline(plot)

    vtag.setFirstAired(ep.get('air_date', ''))
    vtag.setPremiered(ep.get('air_date', ''))
    vtag.setMediaType('episode')

    # Episode IDs
    ep_ids = {}
    ep_tmdb_id = ep.get('id')
    if ep_tmdb_id:
        ep_ids['tmdb'] = str(ep_tmdb_id)
    ext = ep.get('external_ids', {})
    if ext.get('imdb_id'):
        ep_ids['imdb'] = ext['imdb_id']
    if ext.get('tvdb_id'):
        ep_ids['tvdb'] = str(ext['tvdb_id'])
    vtag.setUniqueIDs(ep_ids, 'tmdb')

    # Ratings
    settings = settings or {}
    default = settings.get('default_rating', 'TMDb')
    ep_imdb_id = ext.get('imdb_id', '')

    vote_avg = ep.get('vote_average')
    if vote_avg and (default == 'TMDb' or settings.get('tmdb_anyway')):
        vtag.setRating(
            float(vote_avg), int(ep.get('vote_count') or 0),
            'tmdb', default == 'TMDb',
        )

    # Trakt uses show IMDB ID, not episode IMDB ID
    if show_imdb_id and (default == 'Trakt' or settings.get('trakt_anyway')):
        trakt = trakt_episode(show_imdb_id, season_num, episode_num)
        if trakt:
            vtag.setRating(
                trakt[0], trakt[1], 'trakt', default == 'Trakt',
            )

    if ep_imdb_id and (default == 'IMDb' or settings.get('imdb_anyway')):
        imdb = imdb_rating(ep_imdb_id)
        if imdb:
            vtag.setRating(imdb[0], imdb[1], 'imdb', default == 'IMDb')

    # Stills
    img_base = get_image_base()
    for still in ep.get('images', {}).get('stills', []):
        path = still.get('file_path')
        if path and not path.endswith('.svg'):
            vtag.addAvailableArtwork(
                '{}original{}'.format(img_base, path),
                arttype='thumb',
                preview='{}w780{}'.format(img_base, path),
            )

    # Crew (deduplicated, matching show-level pattern)
    crew = ep.get('crew', [])
    directors = []
    writers = []
    seen_d = set()
    seen_w = set()
    for c in crew:
        n = c.get('name', '')
        if not n:
            continue
        if c.get('job') == 'Director' and n not in seen_d:
            directors.append(n)
            seen_d.add(n)
        if (c.get('department') == 'Writing' or c.get('job') == 'Writer') \
                and n not in seen_w:
            writers.append(n)
            seen_w.add(n)
    if directors:
        vtag.setDirectors(directors)
    if writers:
        vtag.setWriters(writers)

    runtime = ep.get('runtime')
    if runtime:
        vtag.setDuration(runtime * 60)

    # Kodi merges show cast into every episode, regulars already there
    cast = [_make_actor(g) for g in ep.get('guest_stars', []) if g.get('name')]
    if cast:
        vtag.setCast(cast)


def _set_trailer(vtag, show, settings):
    videos = show.get('videos', {}).get('results', [])
    lang = settings.get('lang_details', 'en-US')[:2]
    player = settings.get('trailer_player', 'Tubed')

    if player == 'YouTube':
        base = 'plugin://plugin.video.youtube/play/?video_id={}'
    else:
        base = 'plugin://plugin.video.tubed/?mode=play&video_id={}'

    fallback = [lang] if lang == 'en' else [lang, 'en']
    for try_lang in fallback + [None]:
        for v in videos:
            if v.get('site') != 'YouTube' or v.get('type') != 'Trailer':
                continue
            if try_lang and v.get('iso_639_1') != try_lang:
                continue
            key = v.get('key', '')
            if key:
                vtag.setTrailer(base.format(key))
                return
