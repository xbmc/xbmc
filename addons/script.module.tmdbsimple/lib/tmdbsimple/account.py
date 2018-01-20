# -*- coding: utf-8 -*-

"""
tmdbsimple.account
~~~~~~~~~~~~~~~~~~
This module implements the Account, Authentication, and Lists functionality 
of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB


class Account(TMDB):
    """
    Account functionality.

    See: http://docs.themoviedb.apiary.io/#account
         https://www.themoviedb.org/documentation/api/sessions
    """
    BASE_PATH = 'account'
    URLS = {
        'info': '',
        'lists': '/{id}/lists', 
        'favorite_movies': '/{id}/favorite/movies',
        'favorite_tv': '/{id}/favorite/tv',
        'favorite': '/{id}/favorite',
        'rated_movies': '/{id}/rated/movies',
        'rated_tv': '/{id}/rated/tv',
        'watchlist_movies': '/{id}/watchlist/movies',
        'watchlist_tv': '/{id}/watchlist/tv',
        'watchlist': '/{id}/watchlist',
    }

    def __init__(self, session_id):
        super(Account, self).__init__()
        self.session_id = session_id

    def info(self, **kwargs):
        """
        Get the basic information for an account.

        Call this method first, before calling other Account methods.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('info')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self.id = response['id']
        self._set_attrs_to_values(response)
        return response
        
    def lists(self, **kwargs):
        """
        Get the lists that you have created and marked as a favorite.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('lists')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def favorite_movies(self, **kwargs):
        """
        Get the list of favorite movies for an account.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('favorite_movies')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def favorite_tv(self, **kwargs):
        """
        Get the list of favorite TV series for an account.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('favorite_tv')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def favorite(self, **kwargs):
        """
        Add or remove a movie to an accounts favorite list.

        Args:
            media_type: 'movie' | 'tv'
            media_id: The id of the media.
            favorite: True (to add) | False (to remove).

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('favorite')
        kwargs.update({'session_id': self.session_id})

        payload = {
            'media_type': kwargs.pop('media_type', None), 
            'media_id': kwargs.pop('media_id', None), 
            'favorite': kwargs.pop('favorite', None),
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

    def rated_movies(self, **kwargs):
        """
        Get the list of rated movies (and associated rating) for an account.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('rated_movies')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def rated_tv(self, **kwargs):
        """
        Get the list of rated TV shows (and associated rating) for an account.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('rated_tv')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def watchlist_movies(self, **kwargs):
        """
        Get the list of movies on an account watchlist.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('watchlist_movies')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def watchlist_tv(self, **kwargs):
        """
        Get the list of TV series on an account watchlist.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('watchlist_tv')
        kwargs.update({'session_id': self.session_id})

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def watchlist(self, **kwargs):
        """
        Add or remove a movie to an accounts watch list.

        Args:
            media_type: 'movie' | 'tv'
            media_id: The id of the media.
            watchlist: True (to add) | False (to remove).

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('watchlist')
        kwargs.update({'session_id': self.session_id})

        payload = {
            'media_type': kwargs.pop('media_type', None), 
            'media_id': kwargs.pop('media_id', None), 
            'watchlist': kwargs.pop('watchlist', None),
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

class Authentication(TMDB):
    """
    Authentication functionality.

    See: http://docs.themoviedb.apiary.io/#authentication
         https://www.themoviedb.org/documentation/api/sessions
    """
    BASE_PATH = 'authentication'
    URLS = {
        'token_new': '/token/new',
        'token_validate_with_login': '/token/validate_with_login',
        'session_new': '/session/new', 
        'guest_session_new': '/guest_session/new', 
    }

    def token_new(self, **kwargs):
        """
        Generate a valid request token for user based authentication.

        A request token is required to ask the user for permission to
        access their account.

        After obtaining the request_token, either:
        (1) Direct your user to:
                https://www.themoviedb.org/authenticate/REQUEST_TOKEN
        or:
        (2) Call token_validate_with_login() below.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('token_new')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def token_validate_with_login(self, **kwargs):
        """
        Authenticate a user with a TMDb username and password.  The user
        must have a verified email address and be registered on TMDb.

        Args:
            request_token: The token you generated for the user to approve.
            username: The user's username on TMDb.
            password: The user's password on TMDb.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('token_validate_with_login')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def session_new(self, **kwargs):
        """
        Generate a session id for user based authentication.

        A session id is required in order to use any of the write methods.

        Args:
            request_token: The token you generated for the user to approve.
                           The token needs to be approved before being
                           used here.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('session_new')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def guest_session_new(self, **kwargs):
        """
        Generate a guest session id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('guest_session_new')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class GuestSessions(TMDB):
    """
    Guest Sessions functionality.

    See: http://docs.themoviedb.apiary.io/#guestsessions
    """
    BASE_PATH = 'guest_session'
    URLS = {
        'rated_movies': '/{guest_session_id}/rated_movies',
    }

    def __init__(self, guest_session_id=0):
        super(GuestSessions, self).__init__()
        self.guest_session_id = guest_session_id

    def rated_movies(self, **kwargs):
        """
        Get a list of rated moview for a specific guest session id.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            sort_by: (optional) 'created_at.asc' | 'created_at.desc'
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_guest_session_id_path('rated_movies')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
    

class Lists(TMDB):
    """
    Lists functionality.

    See: http://docs.themoviedb.apiary.io/#lists
    """
    BASE_PATH = 'list'
    URLS = {
        'info': '/{id}',
        'item_status': '/{id}/item_status', 
        'create_list': '',
        'add_item': '/{id}/add_item',
        'remove_item': '/{id}/remove_item',
        'clear': '/{id}/clear',
        'delete_list': '/{id}',
    }

    def __init__(self, id=0, session_id=0):
        super(Lists, self).__init__()
        self.id = id
        self.session_id = session_id

    def info(self, **kwargs):
        """
        Get a list by id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def item_status(self, **kwargs):
        """
        Check to see if a movie id is already added to a list.

        Args:
            movie_id: The id of the movie.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('item_status')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def create_list(self, **kwargs):
        """
        Create a new list.

        A valid session id is required.

        Args:
            name: Name of the list.
            description: Description of the list.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('create_list')
        kwargs.update({'session_id': self.session_id})

        payload = {
            'name': kwargs.pop('name', None), 
            'description': kwargs.pop('description', None),
        }
        if 'language' in kwargs:
            payload['language'] = kwargs['language']

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

    def add_item(self, **kwargs):
        """
        Add new movies to a list that the user created.

        A valid session id is required.

        Args:
            media_id: A movie id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('add_item')
        kwargs.update({'session_id': self.session_id})

        payload = {
            'media_id': kwargs.pop('media_id', None), 
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response
        
    def remove_item(self, **kwargs):
        """
        Delete movies from a list that the user created.

        A valid session id is required.

        Args:
            media_id: A movie id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('remove_item')
        kwargs.update({'session_id': self.session_id})

        payload = {
            'media_id': kwargs.pop('media_id', None), 
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

    def clear_list(self, **kwargs):
        """
        Clears all of the items within a list. This is an irreversible action
        and should be treated with caution.

        A valid session id is required.

        Args:
            confirm: True (do it) | False (don't do it)

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('clear')
        kwargs.update({'session_id': self.session_id})

        payload = {}

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

    def delete_list(self, **kwargs):
        """
        Delete a list that the user created.

        A valid session id is required.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('delete_list')
        kwargs.update({'session_id': self.session_id})

        response = self._DELETE(path, kwargs)
        self._set_attrs_to_values(response)
        return response

