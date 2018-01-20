# -*- coding: utf-8 -*-

"""
tmdbsimple.find
~~~~~~~~~~~~~~~
This module implements the Find functionality of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB

class Find(TMDB):
    """
    Find functionality.

    See: http://docs.themoviedb.apiary.io/#find
    """
    BASE_PATH = 'find'
    URLS = {
        'info': '/{id}',
    }
    def __init__(self, id=0):
        super(Find, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Search for objects in the database by an external id. For instance,
        an IMDB ID. This will search all objects (movies, TV shows and people) 
        and return the results in a single response. TV season and TV episode 
        searches will be supported shortly.

        The supported external sources for each object are as follows:
            Movies: imdb_id
            People: imdb_id, freebase_mid, freebase_id, tvrage_id
            TV Series: imdb_id, freebase_mid, freebase_id, tvdb_id, tvrage_id

        Args:
            external_source: See lists above.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response
