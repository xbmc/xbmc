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
    configure_tmdb_artwork, is_fanarttv_configured

ADDON_SETTINGS = xbmcaddon.Addon()
ID = ADDON_SETTINGS.getAddonInfo('id')

def log(msg, level=xbmc.LOGDEBUG):
    xbmc.log(msg='[{addon}]: {msg}'.format(addon=ID, msg=msg), level=level)

def get_tmdb_scraper(settings):
    language = settings.getSettingString('language')
    certcountry = settings.getSettingString('tmdbcertcountry')
    search_language = settings.getSettingString('searchlanguage')
    return TMDBMovieScraper(ADDON_SETTINGS, language, certcountry, search_language)

def search_for_movie(title, year, handle, settings):
    log("Find movie with title '{title}' from year '{year}'".format(title=title, year=year), xbmc.LOGINFO)
    title = _strip_trailing_article(title)
    scraper = get_tmdb_scraper(settings)

    search_results = scraper.search(title, year)
    if year is not None:
        if not search_results:
            search_results = scraper.search(title,str(int(year)-1))
        if not search_results:
            search_results = scraper.search(title,str(int(year)+1))
        if not search_results:
            search_results = scraper.search(title)
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
    movie_label = movie['title']

    movie_year = movie['release_date'].split('-')[0] if movie.get('release_date') else None
    if movie_year:
        movie_label += ' ({})'.format(movie_year)

    listitem = xbmcgui.ListItem(movie_label, offscreen=True)

    infotag = listitem.getVideoInfoTag()
    infotag.setTitle(movie['title'])
    if movie_year:
        infotag.setYear(int(movie_year))

    if movie['poster_path']:
        listitem.setArt({'thumb': movie['poster_path']})

    return listitem

# Default limit of 10 because a big list of artwork can cause trouble in some cases
# (a column can be too large for the MySQL integration),
# and how useful is a big list anyway? Not exactly rhetorical, this is an experiment.
def add_artworks(listitem, artworks, IMAGE_LIMIT):
    infotag = listitem.getVideoInfoTag()
    for arttype, artlist in artworks.items():
        if arttype == 'fanart':
            continue
        for image in artlist[:IMAGE_LIMIT]:
            infotag.addAvailableArtwork(image['url'], arttype)

    fanart_to_set = [{'image': image['url'], 'preview': image['preview']}
        for image in artworks.get('fanart', ())[:IMAGE_LIMIT]]
    listitem.setAvailableFanart(fanart_to_set)

def get_details(input_uniqueids, handle, settings, fail_silently=False):
    if not input_uniqueids:
        return False
    details = get_tmdb_scraper(settings).get_details(input_uniqueids)
    if not details:
        return False
    if 'error' in details:
        if fail_silently:
            return False
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
        details = combine_scraped_details_available_artwork(details,
            fanarttv_info,
            settings.getSettingString('language'),
            settings)

    details = configure_scraped_details(details, settings)

    listitem = xbmcgui.ListItem(details['info']['title'], offscreen=True)
    infotag = listitem.getVideoInfoTag()
    set_info(infotag, details['info'])
    infotag.setCast(build_cast(details['cast']))
    infotag.setUniqueIDs(details['uniqueids'], 'tmdb')
    infotag.setRatings(build_ratings(details['ratings']), find_defaultrating(details['ratings']))
    IMAGE_LIMIT = settings.getSettingInt('maxartwork')
    add_artworks(listitem, details['available_art'], IMAGE_LIMIT)

    xbmcplugin.setResolvedUrl(handle=handle, succeeded=True, listitem=listitem)
    return True

def set_info(infotag: xbmc.InfoTagVideo, info_dict):
    infotag.setTitle(info_dict['title'])
    infotag.setOriginalTitle(info_dict['originaltitle'])
    infotag.setPlot(info_dict['plot'])
    infotag.setTagLine(info_dict['tagline'])
    infotag.setStudios(info_dict['studio'])
    infotag.setGenres(info_dict['genre'])
    infotag.setCountries(info_dict['country'])
    infotag.setWriters(info_dict['credits'])
    infotag.setDirectors(info_dict['director'])
    infotag.setPremiered(info_dict['premiered'])
    if 'tag' in info_dict:
        infotag.setTags(info_dict['tag'])
    if 'mpaa' in info_dict:
        infotag.setMpaa(info_dict['mpaa'])
    if 'trailer' in info_dict:
        infotag.setTrailer(info_dict['trailer'])
    if 'set' in info_dict:
        infotag.setSet(info_dict['set'])
        infotag.setSetOverview(info_dict['setoverview'])
    if 'duration' in info_dict:
        infotag.setDuration(info_dict['duration'])
    if 'top250' in info_dict:
        infotag.setTop250(info_dict['top250'])

def build_cast(cast_list):
    return [xbmc.Actor(cast['name'], cast['role'], cast['order'], cast['thumbnail']) for cast in cast_list]

def build_ratings(rating_dict):
    return {key: (value['rating'], value.get('votes', 0)) for key, value in rating_dict.items()}

def find_defaultrating(rating_dict):
    return next((key for key, value in rating_dict.items() if value['default']), None)

def find_uniqueids_in_nfo(nfo, handle):
    uniqueids = find_uniqueids_in_text(nfo)
    if uniqueids:
        listitem = xbmcgui.ListItem(offscreen=True)
        xbmcplugin.addDirectoryItem(
            handle=handle, url=build_lookup_string(uniqueids), listitem=listitem, isFolder=True)

def build_lookup_string(uniqueids):
    return json.dumps(uniqueids)

def parse_lookup_string(uniqueids):
    try:
        return json.loads(uniqueids)
    except ValueError:
        log("Can't parse this lookup string, is it from another add-on?\n" + uniqueids, xbmc.LOGWARNING)
        return None

def run():
    params = get_params(sys.argv[1:])
    enddir = True
    if 'action' in params:
        settings = ADDON_SETTINGS if not params.get('pathSettings') else \
            PathSpecificSettings(json.loads(params['pathSettings']), lambda msg: log(msg, xbmc.LOGWARNING))
        action = params["action"]
        if action == 'find' and 'title' in params:
            search_for_movie(params["title"], params.get("year"), params['handle'], settings)
        elif action == 'getdetails' and ('url' in params or 'uniqueIDs' in params):
            unique_ids = parse_lookup_string(params.get('uniqueIDs') or params.get('url'))
            enddir = not get_details(unique_ids, params['handle'], settings, fail_silently='uniqueIDs' in params)
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
