# -*- coding: utf-8 -*-

"""
tmdbsimple.changes
~~~~~~~~~~~~~~~~~~
This module implements the Changes functionality of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB

class Changes(TMDB):
    """
    Changes functionality.

    See: http://docs.themoviedb.apiary.io/#changes
    """
    BASE_PATH = ''
    URLS = {
        'movie': 'movie/changes',
        'person': 'person/changes',
        'tv': 'tv/changes',
    }

    def movie(self, **kwargs):
        """
        Get a list of movie ids that have been edited.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            start_date: (optional) Expected format is 'YYYY-MM-DD'.
            end_date: (optional) Expected format is 'YYYY-MM-DD'.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('movie')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def person(self, **kwargs):
        """
        Get a list of people ids that have been edited.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            start_date: (optional) Expected format is 'YYYY-MM-DD'.
            end_date: (optional) Expected format is 'YYYY-MM-DD'.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('person')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def tv(self, **kwargs):
        """
        Get a list of TV show ids that have been edited.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            start_date: (optional) Expected format is 'YYYY-MM-DD'.
            end_date: (optional) Expected format is 'YYYY-MM-DD'.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('tv')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
