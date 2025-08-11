from datetime import datetime, timedelta
from . import tmdbapi

class TMDBMovieScraper(object):
    def __init__(self, url_settings, language, certification_country, search_language=""):
        self.url_settings = url_settings
        self.language = language
        self.certification_country = certification_country
        if(search_language == ""):
            self.search_language = language
        else:
            self.search_language = search_language
        self._urls = None

    @property
    def urls(self):
        if not self._urls:
            self._urls = _load_base_urls(self.url_settings)
        return self._urls

    def search(self, title, year=None):

        def is_best(item):
            return item['title'].lower() == title and (
                not year or item.get('release_date', '').startswith(year))

        search_media_id = _parse_media_id(title)
        if search_media_id:
            if search_media_id['type'] == 'tmdb':
                result = _get_movie(search_media_id['id'], None, True)
                if 'error' in result:
                    return result
                result = [result]
            else:
                result = tmdbapi.find_movie_by_external_id(search_media_id['id'], language=self.search_language)
                if 'error' in result:
                    return result
                result = result.get('movie_results')
        else:
            response = tmdbapi.search_movie(query=title, year=year, language=self.search_language)
            if 'error' in response:
                return response
            result = response['results']
            # get second page if available and if first page doesn't contain an `is_best` result with popularity > 5
            if response['total_pages'] > 1:
                bests = [item for item in result if is_best(item) and item.get('popularity',0) > 5]
                if not bests:
                    response = tmdbapi.search_movie(query=title, year=year, language=self.language, page=2)
                    if not 'error' in response:
                        result += response['results']
        urls = self.urls

        if result:
            # move all `is_best` results at the beginning of the list, sort them by popularity (if found):
            bests_first = sorted([item for item in result if is_best(item)], key=lambda k: k.get('popularity',0), reverse=True)
            result = bests_first + [item for item in result if item not in bests_first]

        for item in result:
            if item.get('poster_path'):
                item['poster_path'] = urls['preview'] + item['poster_path']
            if item.get('backdrop_path'):
                item['backdrop_path'] = urls['preview'] + item['backdrop_path']
        return result

    def get_details(self, uniqueids):
        media_id = uniqueids.get('tmdb')
        if not media_id:
            imdb_id = uniqueids.get('imdb')
            if not imdb_id:
                return None

            find_results = tmdbapi.find_movie_by_external_id(imdb_id)
            if 'error' in find_results:
                return find_results
            if find_results.get('movie_results'):
                movie = find_results['movie_results'][0]
                media_id = movie['id']
            if not media_id:
                return None

        details = self._gather_details(media_id)
        if not details:
            return None
        if details.get('error'):
            return details
        return self._assemble_details(**details)

    def _gather_details(self, media_id):
        movie = _get_movie(media_id, self.language)
        if not movie or movie.get('error'):
            return movie

        # don't specify language to get English text for fallback
        movie_fallback = _get_movie(media_id)
        movie['images'] = movie_fallback['images']

        collection = _get_moviecollection(movie['belongs_to_collection'].get('id'), self.language) if \
            movie['belongs_to_collection'] else None
        collection_fallback = _get_moviecollection(movie['belongs_to_collection'].get('id')) if \
            movie['belongs_to_collection'] else None
        if collection and collection_fallback and 'images' in collection_fallback:
            collection['images'] = collection_fallback['images']

        return {'movie': movie, 'movie_fallback': movie_fallback, 'collection': collection,
            'collection_fallback': collection_fallback}

    def _assemble_details(self, movie, movie_fallback, collection, collection_fallback):
        info = {
            'title': movie['title'],
            'originaltitle': movie['original_title'],
            'plot': movie.get('overview') or movie_fallback.get('overview'),
            'tagline': movie.get('tagline') or movie_fallback.get('tagline'),
            'studio': _get_names(movie['production_companies']),
            'genre': _get_names(movie['genres']),
            'country': _get_names(movie['production_countries']),
            'credits': _get_cast_members(movie['casts'], 'crew', 'Writing', ['Screenplay', 'Writer', 'Author']),
            'director': _get_cast_members(movie['casts'], 'crew', 'Directing', ['Director']),
            'premiered': movie['release_date'],
            'tag': _get_names(movie['keywords']['keywords'])
        }

        if 'countries' in movie['releases']:
            certcountry = self.certification_country.upper()
            for country in movie['releases']['countries']:
                if country['iso_3166_1'] == certcountry and country['certification']:
                    info['mpaa'] = country['certification']
                    break

        trailer = _parse_trailer(movie.get('trailers', {}), movie_fallback.get('trailers', {}))
        if trailer:
            info['trailer'] = trailer
        if collection:
            info['set'] = collection.get('name') or collection_fallback.get('name')
            info['setoverview'] = collection.get('overview') or collection_fallback.get('overview')
        if movie.get('runtime'):
            info['duration'] = movie['runtime'] * 60

        ratings = {'themoviedb': {'rating': float(movie['vote_average']), 'votes': int(movie['vote_count'])}}
        uniqueids = {'tmdb': str(movie['id']), 'imdb': movie['imdb_id']}
        cast = [{
                'name': actor['name'],
                'role': actor['character'],
                'thumbnail': self.urls['original'] + actor['profile_path']
                    if actor['profile_path'] else "",
                'order': actor['order']
            }
            for actor in movie['casts'].get('cast', [])
        ]
        available_art = _parse_artwork(movie, collection, self.urls, self.language)

        _info = {'set_tmdbid': movie['belongs_to_collection'].get('id')
            if movie['belongs_to_collection'] else None}

        return {'info': info, 'ratings': ratings, 'uniqueids': uniqueids, 'cast': cast,
            'available_art': available_art, '_info': _info}

def _parse_media_id(title):
    if title.startswith('tt') and title[2:].isdigit():
        return {'type': 'imdb', 'id':title} # IMDB ID works alone because it is clear
    title = title.lower()
    if title.startswith('tmdb/') and title[5:].isdigit(): # TMDB ID
        return {'type': 'tmdb', 'id':title[5:]}
    elif title.startswith('imdb/tt') and title[7:].isdigit(): # IMDB ID with prefix to match
        return {'type': 'imdb', 'id':title[5:]}
    return None

def _get_movie(mid, language=None, search=False):
    details = None if search else \
        'trailers,images,releases,casts,keywords' if language is not None else \
        'trailers,images'
    return tmdbapi.get_movie(mid, language=language, append_to_response=details)

def _get_moviecollection(collection_id, language=None):
    if not collection_id:
        return None
    details = 'images'
    return tmdbapi.get_collection(collection_id, language=language, append_to_response=details)

def _parse_artwork(movie, collection, urlbases, language):
    if language:
        # Image languages don't have regional variants
        language = language.split('-')[0]
    posters = []
    landscape = []
    logos = []
    fanart = []

    if 'images' in movie:
        posters = _get_images_with_fallback(movie['images']['posters'], urlbases, language)
        landscape = _get_images_with_fallback(movie['images']['backdrops'], urlbases, language)
        logos = _get_images_with_fallback(movie['images']['logos'], urlbases, language)
        fanart = _get_images(movie['images']['backdrops'], urlbases, None)

    setposters = []
    setlandscape = []
    setfanart = []
    if collection and 'images' in collection:
        setposters = _get_images_with_fallback(collection['images']['posters'], urlbases, language)
        setlandscape = _get_images_with_fallback(collection['images']['backdrops'], urlbases, language)
        setfanart = _get_images(collection['images']['backdrops'], urlbases, None)

    return {'poster': posters, 'landscape': landscape, 'fanart': fanart,
        'set.poster': setposters, 'set.landscape': setlandscape, 'set.fanart': setfanart, 'clearlogo': logos}

def _get_images_with_fallback(imagelist, urlbases, language, language_fallback='en'):
    images = _get_images(imagelist, urlbases, language)

    # Add backup images
    if language != language_fallback:
        images.extend(_get_images(imagelist, urlbases, language_fallback))

    # Add any images if nothing set so far
    if not images:
        images = _get_images(imagelist, urlbases)

    return images

def _get_images(imagelist, urlbases, language='_any'):
    result = []
    for img in imagelist:
        if language != '_any' and img['iso_639_1'] != language:
            continue
        if img['file_path'].endswith('.svg'):
            continue
        result.append({
            'url': urlbases['original'] + img['file_path'],
            'preview': urlbases['preview'] + img['file_path'],
            'lang': img['iso_639_1']
        })
    return result

def _get_date_numeric(datetime_):
    return (datetime_ - datetime(1970, 1, 1)).total_seconds()

def _load_base_urls(url_settings):
    urls = {}
    urls['original'] = url_settings.getSettingString('originalUrl')
    urls['preview'] = url_settings.getSettingString('previewUrl')
    last_updated = url_settings.getSettingString('lastUpdated')
    if not urls['original'] or not urls['preview'] or not last_updated or \
            float(last_updated) < _get_date_numeric(datetime.now() - timedelta(days=30)):
        conf = tmdbapi.get_configuration()
        if conf:
            urls['original'] = conf['images']['secure_base_url'] + 'original'
            urls['preview'] = conf['images']['secure_base_url'] + 'w780'
            url_settings.setSetting('originalUrl', urls['original'])
            url_settings.setSetting('previewUrl', urls['preview'])
            url_settings.setSetting('lastUpdated', str(_get_date_numeric(datetime.now())))
    return urls

def _parse_trailer(trailers, fallback):
    if trailers.get('youtube'):
        return 'plugin://plugin.video.youtube/play/?video_id='+trailers['youtube'][0]['source']
    if fallback.get('youtube'):
        return 'plugin://plugin.video.youtube/play/?video_id='+fallback['youtube'][0]['source']
    return None

def _get_names(items):
    return [item['name'] for item in items] if items else []

def _get_cast_members(casts, casttype, department, jobs):
    result = []
    if casttype in casts:
        for cast in casts[casttype]:
            if cast['department'] == department and cast['job'] in jobs and cast['name'] not in result:
                result.append(cast['name'])
    return result
