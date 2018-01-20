# -*- coding: utf-8 -*-

"""
tmdbsimple.people
~~~~~~~~~~~~~~~~~
This module implements the People, Credits, and Jobs functionality 
of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB

class People(TMDB):
    """
    People functionality.

    See: http://docs.themoviedb.apiary.io/#people
    """
    BASE_PATH = 'person'
    URLS = {
        'info': '/{id}',
        'movie_credits': '/{id}/movie_credits',
        'tv_credits': '/{id}/tv_credits',
        'combined_credits': '/{id}/combined_credits',
        'external_ids': '/{id}/external_ids',
        'images': '/{id}/images',
        'changes': '/{id}/changes',
        'popular': '/popular',
        'latest': '/latest',
    }

    def __init__(self, id=0):
        super(People, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the general person information for a specific id.

        Args:
            append_to_response: (optional) Comma separated, any person method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def movie_credits(self, **kwargs):
        """
        Get the movie credits for a specific person id.

        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any person method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('movie_credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def tv_credits(self, **kwargs):
        """
        Get the TV credits for a specific person id.

        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any person method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('tv_credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def combined_credits(self, **kwargs):
        """
        Get the combined (movie and TV) credits for a specific person id.

        To get the expanded details for each TV record, call the /credit method 
        with the provided credit_id. This will provide details about which 
        episode and/or season the credit is for.

        Args:
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any person method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('combined_credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def external_ids(self, **kwargs):
        """
        Get the external ids for a specific person id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('external_ids')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get the images for a specific person id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('images')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def changes(self, **kwargs):
        """
        Get the changes for a specific person id.

        Changes are grouped by key, and ordered by date in descending order. 
        By default, only the last 24 hours of changes are returned. The maximum 
        number of days that can be returned in a single request is 14. The 
        language is present on fields that are translatable.

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

    def popular(self, **kwargs):
        """
        Get the list of popular people on The Movie Database. This list 
        refreshes every day.

        Args:
            page: (optional) Minimum 1, maximum 1000.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('popular')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def latest(self, **kwargs):
        """
        Get the latest person id.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('latest')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class Credits(TMDB):
    """
    Credits functionality.

    See: http://docs.themoviedb.apiary.io/#credits
    """
    BASE_PATH = 'credit'
    URLS = {
        'info': '/{credit_id}',
    }

    def __init__(self, credit_id):
        super(Credits, self).__init__()
        self.credit_id = credit_id

    def info(self, **kwargs):
        """
        Get the detailed information about a particular credit record. This is 
        currently only supported with the new credit model found in TV. These 
        ids can be found from any TV credit response as well as the tv_credits 
        and combined_credits methods for people.

        The episodes object returns a list of episodes and are generally going 
        to be guest stars. The season array will return a list of season 
        numbers.  Season credits are credits that were marked with the 
        "add to every season" option in the editing interface and are 
        assumed to be "season regulars".

        Args:
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_credit_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

class Jobs(TMDB):
    """
    Jobs functionality.

    See: http://docs.themoviedb.apiary.io/#jobs
    """
    BASE_PATH = 'job'
    URLS = {
        'list': '/list',
    }

    def list(self, **kwargs):
        """
        Get a list of valid jobs.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('list')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
        
