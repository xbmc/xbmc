# SPDX-License-Identifier: GPL-3.0-or-later

"""Artwork classification, scoring, and byte-aware selection.

Images are classified by art type, ranked by language tier then
resolution, and limited to byte budgets (c06/c11) to enforce
MySQL TEXT limit.
"""

from lib.api.tmdb import get_image_base

# MySQL TEXT = 65,535 bytes
_C06_BUDGET = 64000
_C11_BUDGET = 64000
_C11_WRAPPER = 17  # <fanart></fanart>

_SEASON_MIN = 2
_SHOW_MIN = 10
_TEXT_BEARING = frozenset(('poster', 'landscape', 'clearlogo', 'banner', 'clearart'))


def set_artwork(li, show_info, settings):
    """Main entry: classify, score, select, output to ListItem."""
    vtag = li.getVideoInfoTag()
    user_lang = settings['lang_images'][:2].lower()
    cat_kart = settings.get('cat_keyart', True)
    cat_land = settings.get('cat_landscape', True)

    candidates = []
    _classify_images(
        candidates, show_info.get('images', {}),
        None, cat_kart, cat_land,
    )
    for season in show_info.get('seasons', []):
        snum = season.get('season_number', 0)
        simages = season.get('images')
        if simages:
            _classify_images(
                candidates, simages,
                snum, cat_kart, cat_land,
            )

    prefer_maxres = settings.get('prefer_maxres', False)
    prefer_ftv_logos = settings.get('fanarttv_prefer_logos', True)
    prefer_ftv_art = settings.get('fanarttv_prefer_art', False)
    for c in candidates:
        is_ftv = c.get('fanarttv', False)
        if c['art_type'] == 'clearlogo':
            pixels = min((c.get('width') or 0) * (c.get('height') or 0), 800 * 310)
            source_pref = is_ftv and prefer_ftv_logos
        else:
            pixels = (c.get('width') or 0) * (c.get('height') or 0)
            source_pref = is_ftv and prefer_ftv_art
        base = (source_pref, pixels) if prefer_maxres else (source_pref,)
        if c['art_type'] in _TEXT_BEARING:
            lang_tier = 4 - _lang_sort_key(c.get('language'), user_lang)[0]
            c['score'] = (lang_tier,) + base
        else:
            c['score'] = base

    c06 = [c for c in candidates if c['column'] == 'c06']
    c11 = [c for c in candidates if c['column'] == 'c11']
    keep_c06 = _select(c06, _C06_BUDGET)
    keep_c11 = _select(c11, _C11_BUDGET - _C11_WRAPPER)

    fanart_urls = []
    for c in keep_c11:
        fanart_urls.append({'image': c['url']})
    if fanart_urls:
        try:
            vtag.setAvailableFanart(fanart_urls)
        except AttributeError:
            li.setAvailableFanart(fanart_urls)

    for c in keep_c06:
        kwargs = {'arttype': c['art_type'], 'preview': c['preview']}
        if c['season'] is not None:
            kwargs['season'] = c['season']
        vtag.addAvailableArtwork(c['url'], **kwargs)


def _classify_images(candidates, images, season, cat_kart, cat_land):
    """Classify images and append to candidates list."""
    base = get_image_base()
    w500 = base + 'w500'
    w780 = base + 'w780'

    for raw in images.get('posters', []):
        entry = _make_entry(raw, base, w500)
        if not entry:
            continue
        lang = raw.get('iso_639_1')
        if (lang is None or lang == 'xx') and cat_kart:
            entry.update(art_type='keyart', column='c06', season=season)
        else:
            entry.update(art_type='poster', column='c06', season=season)
        candidates.append(entry)

    for raw in images.get('backdrops', []):
        entry = _make_entry(raw, base, w780)
        if not entry:
            continue
        lang = raw.get('iso_639_1')
        if lang and lang != 'xx' and cat_land:
            entry.update(art_type='landscape', column='c06', season=season)
        else:
            entry.update(art_type='fanart', column='c11', season=season)
        candidates.append(entry)

    for raw in images.get('logos', []):
        entry = _make_entry(raw, base, w500)
        if not entry:
            continue
        entry.update(art_type='clearlogo', column='c06', season=season)
        candidates.append(entry)

    for art_type in ('banner', 'clearart', 'characterart'):
        for raw in images.get(art_type, []):
            entry = _make_entry(raw, base, w500)
            if not entry:
                continue
            entry.update(art_type=art_type, column='c06', season=season)
            candidates.append(entry)

    for raw in images.get('landscape', []):
        entry = _make_entry(raw, base, w780)
        if not entry:
            continue
        entry.update(art_type='landscape', column='c06', season=season)
        candidates.append(entry)


def _select(entries, byte_budget):
    """Pick the best art per type per season, then fill by quality.

    Every (art_type, season) group gets up to 2 entries in the priority
    pool so each season has choices. Show-only types get up to 10.
    Remaining budget fills with the best-scoring leftovers.

    """
    if not entries:
        return []

    # Group by (art_type, season)
    groups = {}
    for e in entries:
        groups.setdefault((e['art_type'], e['season']), []).append(e)

    priority = []
    overflow = []
    for (_, season), group in groups.items():
        group.sort(key=lambda e: e['score'], reverse=True)
        if season is None:
            cap = _SHOW_MIN
        else:
            cap = _SEASON_MIN
        cap = min(cap, len(group))
        priority.extend(group[:cap])
        overflow.extend(group[cap:])

    priority.sort(key=lambda e: e['score'], reverse=True)
    selected = []
    used = 0
    for e in priority:
        cost = _byte_cost(e)
        if used + cost <= byte_budget:
            selected.append(e)
            used += cost

    overflow.sort(key=lambda e: e['score'], reverse=True)
    for e in overflow:
        cost = _byte_cost(e)
        if used + cost <= byte_budget:
            selected.append(e)
            used += cost

    return selected


def _byte_cost(entry):
    """XML serialization cost for one image entry."""
    if entry['column'] == 'c11':
        # <thumb colors="" preview="">URL</thumb>\n
        # setAvailableFanart only receives {'image': url}, Kodi stores preview=""
        return 36 + len(entry['url'])
    # <thumb spoof="" cache="" [season="N" type="season" ]aspect="TYPE" preview="PREVIEW">URL</thumb>
    cost = 54 + len(entry['art_type']) + len(entry['preview']) + len(entry['url'])
    if entry['season'] is not None:
        cost += 24 + len(str(entry['season']))
    return cost


def _make_entry(raw_image, img_base, preview_base):
    """Convert a raw image dict into a candidate entry."""
    path = raw_image.get('file_path')
    if not path or path.endswith('.svg'):
        return None
    if raw_image.get('type') == 'fanarttv':
        url = path
        preview = path.replace('/fanart/', '/preview/', 1)
    else:
        url = '{}original{}'.format(img_base, path)
        preview = '{}{}'.format(preview_base, path)
    return {
        'url': url,
        'preview': preview,
        'language': raw_image.get('iso_639_1'),
        'width': raw_image.get('width') or 0,
        'height': raw_image.get('height') or 0,
        'fanarttv': raw_image.get('type') == 'fanarttv',
    }


def _lang_sort_key(lang, user_lang):
    if lang:
        lang = lang.lower()
    if lang == user_lang:
        return (0,)
    if lang == 'en' and user_lang != 'en':
        return (1,)
    if not lang or lang == 'xx':
        return (2,)
    return (3, lang or '')
