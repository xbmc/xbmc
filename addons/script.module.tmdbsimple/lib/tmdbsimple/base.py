# -*- coding: utf-8 -*-

"""
tmdbsimple.base
~~~~~~~~~~~~~~~
This module implements the base class of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

import json
import requests


class APIKeyError(Exception):
    pass


class TMDB(object):
    headers = {'Content-Type': 'application/json',
               'Accept': 'application/json',
               'Connection': 'close'}
    BASE_PATH = ''
    URLS = {}

    def __init__(self):
        from . import API_VERSION
        self.base_uri = 'https://api.themoviedb.org'
        self.base_uri += '/{version}'.format(version=API_VERSION)

    def _get_path(self, key):
        return self.BASE_PATH + self.URLS[key]

    def _get_id_path(self, key):
        return self._get_path(key).format(id=self.id)

    def _get_guest_session_id_path(self, key):
        return self._get_path(key).format(
            guest_session_id=self.guest_session_id)
    
    def _get_credit_id_path(self, key):
        return self._get_path(key).format(credit_id=self.credit_id)

    def _get_id_season_number_path(self, key):
        return self._get_path(key).format(id=self.id,
            season_number=self.season_number)

    def _get_series_id_season_number_episode_number_path(self, key):
        return self._get_path(key).format(series_id=self.series_id,
            season_number=self.season_number,
            episode_number=self.episode_number)

    def _get_complete_url(self, path):
        return '{base_uri}/{path}'.format(base_uri=self.base_uri, path=path)

    def _get_params(self, params):
        from . import API_KEY
        if not API_KEY:
            raise APIKeyError

        api_dict = {'api_key': API_KEY}
        if params:
            params.update(api_dict)
        else:
            params = api_dict
        return params

    def _request(self, method, path, params=None, payload=None):
        url = self._get_complete_url(path)
        params = self._get_params(params)

        response = requests.request(
            method, url, params=params, 
            data=json.dumps(payload) if payload else payload,
            headers=self.headers)

        response.raise_for_status()
        response.encoding = 'utf-8'
        return response.json()

    def _GET(self, path, params=None):
        return self._request('GET', path, params=params)

    def _POST(self, path, params=None, payload=None):
        return self._request('POST', path, params=params, payload=payload)

    def _DELETE(self, path, params=None, payload=None):
        return self._request('DELETE', path, params=params, payload=payload)

    def _set_attrs_to_values(self, response={}):
        """
        Set attributes to dictionary values.

        - e.g.
        >>> import tmdbsimple as tmdb
        >>> movie = tmdb.Movies(103332)
        >>> response = movie.info()
        >>> movie.title  # instead of response['title']
        """
        if isinstance(response, dict):
            for key in response.keys():
                setattr(self, key, response[key])

