# -*- coding: utf-8 -*-

"""
tmdbsimple.tv
~~~~~~~~~~~~~
This module implements the TV, TV Seasons, TV Episodes, and Networks 
functionality of tmdbsimple.

Created by Celia Oakley on 2013-10-31.

:copyright: (c) 2013-2017 by Celia Oakley
:license: GPLv3, see LICENSE for more details
"""

from .base import TMDB


class TV(TMDB):
    """
    TV functionality.

    See: http://docs.themoviedb.apiary.io/#tv
    """
    BASE_PATH = 'tv'
    URLS = {
        'info': '/{id}',
        'credits': '/{id}/credits',
        'external_ids': '/{id}/external_ids',
        'images': '/{id}/images',
        'rating': '/{id}/rating',
        'similar': '/{id}/similar',
        'recommendations': '/{id}/recommendations',
        'translations': '/{id}/translations',
        'videos': '/{id}/videos',
        'on_the_air': '/on_the_air',
        'airing_today': '/airing_today',
        'top_rated': '/top_rated',
        'popular': '/popular',
    }

    def __init__(self, id=0):
        super(TV, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        Get the primary information about a TV series by id.

        Args:
            language: (optional) ISO 639 code.
            append_to_response: (optional) Comma separated, any TV series 
                                method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def credits(self, **kwargs):
        """
        Get the cast & crew information about a TV series. Just like the 
        website, we pull this information from the last season of the series.

        Args:
            language: (optional) ISO 639 code.
            append_to_response: (optional) Comma separated, any collection 
                                method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def external_ids(self, **kwargs):
        """
        Get the external ids that we have stored for a TV series.

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('external_ids')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get the images (posters and backdrops) for a TV series.

        Args:
            language: (optional) ISO 639 code.
            include_image_language: (optional) Comma separated, a valid 
                                    ISO 69-1.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('images')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def rating(self, **kwargs):
        """
        This method lets users rate a TV show. A valid session id or guest
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

    def similar(self, **kwargs):
        """
        Get the similar TV series for a specific TV series id.

        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.
            append_to_response: (optional) Comma separated, any TV method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('similar')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def recommendations(self, **kwargs):
        """
        Get the recommendations for TV series for a specific TV series id.

        Args:
            page: (optional) Minimum value of 1.  Expected value is an integer.
            language: (optional) ISO 639-1 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('recommendations')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def translations(self, **kwargs):
        """
        Get the list of translations that exist for a TV series. These 
        translations cascade down to the episode level.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('translations')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def videos(self, **kwargs):
        """
        Get the videos that have been added to a TV series (trailers, opening
        credits, etc...).

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('videos')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def on_the_air(self, **kwargs):
        """
        Get the list of TV shows that are currently on the air. This query
        looks for any TV show that has an episode with an air date in the
        next 7 days.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('on_the_air')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def airing_today(self, **kwargs):
        """
        Get the list of TV shows that air today. Without a specified timezone,
        this query defaults to EST (Eastern Time UTC-05:00).

        Args:
            page: (optional) Minimum 1, maximum 1000.
            language: (optional) ISO 639 code.
            timezone: (optional) Valid value from the list of timezones.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('airing_today')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def top_rated(self, **kwargs):
        """
        Get the list of top rated TV shows. By default, this list will only 
        include TV shows that have 2 or more votes. This list refreshes every 
        day.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('top_rated')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def popular(self, **kwargs):
        """
        Get the list of popular TV shows. This list refreshes every day.

        Args:
            page: (optional) Minimum 1, maximum 1000.
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_path('popular')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response


class TV_Seasons(TMDB):
    """
    TV Seasons functionality.

    See: http://docs.themoviedb.apiary.io/#tvseasons
    """
    BASE_PATH = 'tv/{id}/season/{season_number}'
    URLS = {
        'info': '',
        'credits': '/credits',
        'external_ids': '/external_ids',
        'images': '/images',
        'videos': '/videos',
    }

    def __init__(self, id, season_number):
        super(TV_Seasons, self).__init__()
        self.id = id
        self.season_number = season_number

    def info(self, **kwargs):
        """
        Get the primary information about a TV season by its season number.

        Args:
            language: (optional) ISO 639 code.
            append_to_response: (optional) Comma separated, any TV series 
                                method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_season_number_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def credits(self, **kwargs):
        """
        Get the cast & crew credits for a TV season by season number.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_season_number_path('credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def external_ids(self, **kwargs):
        """
        Get the external ids that we have stored for a TV season by season 
        number.

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_season_number_path('external_ids')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get the images (posters) that we have stored for a TV season by season 
        number.

        Args:
            language: (optional) ISO 639 code.
            include_image_language: (optional) Comma separated, a valid 
                                    ISO 69-1.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_season_number_path('images')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def videos(self, **kwargs):
        """
        Get the videos that have been added to a TV season (trailers, teasers,
        etc...).

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_season_number_path('videos')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response


class TV_Episodes(TMDB):
    """
    TV Episodes functionality.

    See: http://docs.themoviedb.apiary.io/#tvepisodes
    """
    BASE_PATH = 'tv/{series_id}/season/{season_number}/episode/{episode_number}'
    URLS = {
        'info': '',
        'credits': '/credits',
        'external_ids': '/external_ids',
        'images': '/images',
        'rating': '/rating',
        'videos': '/videos',
    }

    def __init__(self, series_id, season_number, episode_number):
        super(TV_Episodes, self).__init__()
        self.series_id = series_id
        self.season_number = season_number
        self.episode_number = episode_number

    def info(self, **kwargs):
        """
        Get the primary information about a TV episode by combination of a 
        season and episode number.

        Args:
            language: (optional) ISO 639 code.
            append_to_response: (optional) Comma separated, any TV series 
                                method.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def credits(self, **kwargs):
        """
        Get the TV episode credits by combination of season and episode number.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path('credits')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def external_ids(self, **kwargs):
        """
        Get the external ids for a TV episode by combination of a season and 
        episode number.

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path(
            'external_ids')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def images(self, **kwargs):
        """
        Get the images (episode stills) for a TV episode by combination of a 
        season and episode number. Since episode stills don't have a language, 
        this call will always return all images.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path('images')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

    def rating(self, **kwargs):
        """
        This method lets users rate a TV episode. A valid session id or guest
        session id is required.

        Args:
            session_id: see Authentication.
            guest_session_id: see Authentication.
            value: Rating value.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path('rating')

        payload = {
            'value': kwargs.pop('value', None),
        }

        response = self._POST(path, kwargs, payload)
        self._set_attrs_to_values(response)
        return response

    def videos(self, **kwargs):
        """
        Get the videos that have been added to a TV episode (teasers, clips,
        etc...).

        Args:
            language: (optional) ISO 639 code.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_series_id_season_number_episode_number_path('videos')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response


class Networks(TMDB):
    """
    Networks functionality.

    See: http://docs.themoviedb.apiary.io/#networks
    """
    BASE_PATH = 'network'
    URLS = {
        'info': '/{id}',
    }

    def __init__(self, id):
        super(Networks, self).__init__()
        self.id = id

    def info(self, **kwargs):
        """
        This method is used to retrieve the basic information about a TV 
        network. You can use this ID to search for TV shows with the discover. 
        At this time we don't have much but this will be fleshed out over time.

        Returns:
            A dict respresentation of the JSON returned from the API.
        """
        path = self._get_id_path('info')

        response = self._GET(path, kwargs)
        self._set_attrs_to_values(response)
        return response

