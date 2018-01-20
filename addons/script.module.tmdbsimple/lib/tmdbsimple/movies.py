# -*- coding: utf-8 -*-

"""
tmdbsimple.movies
~~~~~~~~~~~~~~~~~
This module implements the Movies, Collections, Companies, Keywords, and 
Reviews functionality of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB

class Movies(TMDB):
    """
    Movies functionality.

    See: http://docs.themoviedb.apiary.io/#movies
    """
    BASE_PATH = 'movie'
    URLS = {
        'info': '/{id}',
        'alternative_titles': '/{id}/alternative_titles',
        'credits': '/{id}/credits',
        'images': '/{id}/images',
        'keywords': '/{id}/keywords',
        'releases': '/{id}/releases',
        'videos': '/{id}/videos',
        'translations': '/{id}/translations',
        'similar_movies': '/{id}/similar_movies',
        'reviews': '/{id}/reviews',
        'lists': '/{id}/lists',
        'changes': '/{id}/changes',
        'latest': '/latest',
        'upcoming': '/upcoming',
        'now_playing': '/now_playing',
        'popular': '/popular',
        'top_rated': '/top_rated',
        'account_states': '/{id}/account_states',
        'rating': '/{id}/rating',
    }

    def __init__(self, id=0):
        super(Movies, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the basic movie information for a specific movie id.
        
        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def alternative_titles(self, **kwargs):
        """
        Get the alternative titles for a specific movie id.
        
        Args:
            country: (optional) ISO 3166-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('alternative_titles')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def credits(self, **kwargs):
        """
        Get the cast and crew information for a specific movie id.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get the images (posters and backdrops) for a specific movie id.
        
        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.
            include_image_language: (optional) Comma separated, a valid 
                                    ISO 69-1. 

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('images')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def keywords(self, **kwargs):
        """
        Get the plot keywords for a specific movie id.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('keywords')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def releases(self, **kwargs):
        """
        Get the release date and certification information by country for a 
        specific movie id.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('releases')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def videos(self, **kwargs):
        """
        Get the videos (trailers, teasers, clips, etc...) for a 
        specific movie id.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('videos')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def translations(self, **kwargs):
        """
        Get the translations for a specific movie id.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('translations')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def similar_movies(self, **kwargs):
        """
        Get the similar movies for a specific movie id.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('similar_movies')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def reviews(self, **kwargs):
        """
        Get the reviews for a particular movie id.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('reviews')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def lists(self, **kwargs):
        """
        Get the lists that the movie belongs to.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('lists')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def changes(self, **kwargs):
        """
        Get the changes for a specific movie id.

        Changes are grouped by key, and ordered by date in descending order. 
        By default, only the last 24 hours of changes are returned. The 
        maximum number of days that can be returned in a single request is 14. 
        The language is present on fields that are translatable.
        
        Args:
            start_date: (optional) Expected format is 'YYYY-MM-DD'.
            end_date: (optional) Expected format is 'YYYY-MM-DD'.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('changes')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def latest(self, **kwargs):
        """
        Get the latest movie id.
        
        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('latest')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def upcoming(self, **kwargs):
        """
        Get the list of upcoming movies. This list refreshes every day. 
        The maximum number of items this list will include is 100.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('upcoming')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def now_playing(self, **kwargs):
        """
        Get the list of movies playing in theatres. This list refreshes 
        every day. The maximum number of items this list will include is 100.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('now_playing')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def popular(self, **kwargs):
        """
        Get the list of popular movies on The Movie Database. This list 
        refreshes every day.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('popular')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def top_rated(self, **kwargs):
        """
        Get the list of top rated movies. By default, this list will only 
        include movies that have 10 or more votes. This list refreshes every 
        day.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('top_rated')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def account_states(self, **kwargs):
        """
        This method lets users get the status of whether or not the movie has 
        been rated or added to their favourite or watch lists. A valid session 
        id is required.
        
        Args:
            session_id: see Authentication.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('account_states')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def rating(self, **kwargs):
        """
        This method lets users rate a movie. A valid session id or guest 
        session id is required.

        Args:
            session_id: see Authentication.
            guest_session_id: see Authentication.
            value: Rating value.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('rating')

        payload = {
            'value': kwargs.pop('value', None),
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response


class Collections(TMDB):
    """
    Collections functionality. 

    See: http://docs.themoviedb.apiary.io/#collections
    """
    BASE_PATH = 'collection'
    URLS = {
        'info': '/{id}',
        'images': '/{id}/images',
    }

    def __init__(self, id):
        super(Collections, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the basic collection information for a specific collection id. 
        You can get the ID needed for this method by making a /movie/{id} 
        request and paying attention to the belongs_to_collection hash.

        Movie parts are not sorted in any particular order. If you would like 
        to sort them yourself you can use the provided release_date.
        
        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get all of the images for a particular collection by collection id.
        
        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.
            include_image_language: (optional) Comma separated, a valid 
            ISO 69-1. 

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class Companies(TMDB):
    """
    Companies functionality. 

    See: http://docs.themoviedb.apiary.io/#companies
    """
    BASE_PATH = 'company'
    URLS = {
        'info': '/{id}',
        'movies': '/{id}/movies',
    }

    def __init__(self, id=0):
        super(Companies, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        This method is used to retrieve all of the basic information about a 
        company.
        
        Args:
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
        
    def movies(self, **kwargs):
        """
        Get the list of movies associated with a particular company.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any movie method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('movies')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class Keywords(TMDB):
    """
    Keywords functionality. 

    See: http://docs.themoviedb.apiary.io/#keywords
    """
    BASE_PATH = 'keyword'
    URLS = {
        'info': '/{id}',
        'movies': '/{id}/movies',
    }

    def __init__(self, id):
        super(Keywords, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the basic information for a specific keyword id.
        
        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def movies(self, **kwargs):
        """
        Get the list of movies for a particular keyword by id.
        
        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('movies')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class Reviews(TMDB):
    """
    Reviews functionality. 

    See: http://docs.themoviedb.apiary.io/#reviews
    """
    BASE_PATH = 'review'
    URLS = {
        'info': '/{id}',
    }

    def __init__(self, id):
        super(Reviews, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the full details of a review by ID.
        
        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
