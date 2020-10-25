import json
import sys
import xbmc
import xbmcaddon
import xbmcgui
import xbmcplugin

from lib.tmdbscraper.tmdb import TMDBMovieScraper
from lib.tmdbscraper.fanarttv import get_details as get_fanarttv_artwork
from lib.tmdbscraper.imdbratings import get_details as get_imdb_details
from lib.tmdbscraper.traktratings import get_trakt_ratinginfo
from scraper_datahelper import combine_scraped_details_info_and_ratings, \
    combine_scraped_details_available_artwork, find_uniqueids_in_text, get_params
from scraper_config import configure_scraped_details, PathSpecificSettings, \
    configure_fanarttv_artwork, configure_tmdb_artwork, is_fanarttv_configured

ADDON_SETTINGS = xbmcaddon.Addon()
ID = ADDON_SETTINGS.getAddonInfo('id')

def log(msg, level=xbmc.LOGDEBUG):
    xbmc.log(msg='[{addon}]: {msg}'.format(addon=ID, msg=msg), level=level)

def get_tmdb_scraper(settings):
    language = settings.getSettingString('language')
    certcountry = settings.getSettingString('tmdbcertcountry')
    return TMDBMovieScraper(ADDON_SETTINGS, language, certcountry)

def search_for_movie(title, year, handle, settings):
    log("Find movie with title '{title}' from year '{year}'".format(title=title, year=year), xbmc.LOGINFO)
    title = _strip_trailing_article(title)
    search_results = get_tmdb_scraper(settings).search(title, year)
    if not search_results:
        return
    if 'error' in search_results:
        header = "The Movie Database Python error searching with web service TMDB"
        xbmcgui.Dialog().notification(header, search_results['error'], xbmcgui.NOTIFICATION_WARNING)
        log(header + ': ' + search_results['error'], xbmc.LOGWARNING)
        return

    for movie in search_results:
        listitem = _searchresult_to_listitem(movie)
        uniqueids = {'tmdb': str(movie['id'])}
        xbmcplugin.addDirectoryItem(handle=handle, url=build_lookup_string(uniqueids),
            listitem=listitem, isFolder=True)

_articles = [prefix + article for prefix in (', ', ' ') for article in ("the", "a", "an")]
def _strip_trailing_article(title):
    title = title.lower()
    for article in _articles:
        if title.endswith(article):
            return title[:-len(article)]
    return title

def _searchresult_to_listitem(movie):
    movie_info = {'title': movie['title']}
    movie_label = movie['title']

    movie_year = movie['release_date'].split('-')[0] if movie.get('release_date') else None
    if movie_year:
        movie_label += ' ({})'.format(movie_year)
        movie_info['year'] = movie_year

    listitem = xbmcgui.ListItem(movie_label, offscreen=True)

    listitem.setInfo('video', movie_info)
    if movie['poster_path']:
        listitem.setArt({'thumb': movie['poster_path']})

    return listitem

# Low limit because a big list of artwork can cause trouble in some cases
# (a column can be too large for the MySQL integration),
# and how useful is a big list anyway? Not exactly rhetorical, this is an experiment.
IMAGE_LIMIT = 10

def add_artworks(listitem, artworks):
    for arttype, artlist in artworks.items():
        if arttype == 'fanart':
            continue
        for image in artlist[:IMAGE_LIMIT]:
            listitem.addAvailableArtwork(image['url'], arttype)

    fanart_to_set = [{'image': image['url'], 'preview': image['preview']}
        for image in artworks['fanart'][:IMAGE_LIMIT]]
    listitem.setAvailableFanart(fanart_to_set)

def get_details(input_uniqueids, handle, settings):
    details = get_tmdb_scraper(settings).get_details(input_uniqueids)
    if not details:
        return False
    if 'error' in details:
        header = "The Movie Database Python error with web service TMDB"
        xbmcgui.Dialog().notification(header, details['error'], xbmcgui.NOTIFICATION_WARNING)
        log(header + ': ' + details['error'], xbmc.LOGWARNING)
        return False

    details = configure_tmdb_artwork(details, settings)

    if settings.getSettingString('RatingS') == 'IMDb' or settings.getSettingBool('imdbanyway'):
        imdbinfo = get_imdb_details(details['uniqueids'])
        if 'error' in imdbinfo:
            header = "The Movie Database Python error with website IMDB"
            log(header + ': ' + imdbinfo['error'], xbmc.LOGWARNING)
        else:
            details = combine_scraped_details_info_and_ratings(details, imdbinfo)

    if settings.getSettingString('RatingS') == 'Trakt' or settings.getSettingBool('traktanyway'):
        traktinfo = get_trakt_ratinginfo(details['uniqueids'])
        details = combine_scraped_details_info_and_ratings(details, traktinfo)

    if is_fanarttv_configured(settings):
        fanarttv_info = get_fanarttv_artwork(details['uniqueids'],
            settings.getSettingString('fanarttv_clientkey'),
            settings.getSettingString('fanarttv_language'),
            details['_info']['set_tmdbid'])
        fanarttv_info = configure_fanarttv_artwork(fanarttv_info, settings)
        details = combine_scraped_details_available_artwork(details, fanarttv_info)

    details = configure_scraped_details(details, settings)

    listitem = xbmcgui.ListItem(details['info']['title'], offscreen=True)
    listitem.setInfo('video', details['info'])
    listitem.setCast(details['cast'])
    listitem.setUniqueIDs(details['uniqueids'], 'tmdb')
    add_artworks(listitem, details['available_art'])

    for rating_type, value in details['ratings'].items():
        if 'votes' in value:
            listitem.setRating(rating_type, value['rating'], value['votes'], value['default'])
        else:
            listitem.setRating(rating_type, value['rating'], defaultt=value['default'])

    xbmcplugin.setResolvedUrl(handle=handle, succeeded=True, listitem=listitem)
    return True

def find_uniqueids_in_nfo(nfo, handle):
    uniqueids = find_uniqueids_in_text(nfo)
    if uniqueids:
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(
            handle=handle, url=build_lookup_string(uniqueids), listitem=listitem, isFolder=True)

def build_lookup_string(uniqueids):
    return json.dumps(uniqueids)

def parse_lookup_string(uniqueids):
    return json.loads(uniqueids)

def run():
    params = get_params(sys.argv[1:])
    enddir = True
    if 'action' in params:
        settings = ADDON_SETTINGS if not params.get('pathSettings') else \
            PathSpecificSettings(json.loads(params['pathSettings']), lambda msg: log(msg, xbmc.LOGWARNING))
        action = params["action"]
        if action == 'find' and 'title' in params:
            search_for_movie(params["title"], params.get("year"), params['handle'], settings)
        elif action == 'getdetails' and 'url' in params:
            enddir = not get_details(parse_lookup_string(params["url"]), params['handle'], settings)
        elif action == 'NfoUrl' and 'nfo' in params:
            find_uniqueids_in_nfo(params["nfo"], params['handle'])
        else:
            log("unhandled action: " + action, xbmc.LOGWARNING)
    else:
        log("No action in 'params' to act on", xbmc.LOGWARNING)
    if enddir:
        xbmcplugin.endOfDirectory(params['handle'])

if __name__ == '__main__':
    run()
