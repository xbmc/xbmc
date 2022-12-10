# -*- coding: UTF-8 -*-
#
# Copyright (C) 2020, Team Kodi
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# IMDb ratings based on code in metadata.themoviedb.org.python by Team Kodi
# pylint: disable=missing-docstring


import re
import json
from . import api_utils
from . import settings
try:
    from typing import Optional, Tuple, Text, Dict, List, Any  # pylint: disable=unused-import
except ImportError:
    pass

IMDB_RATINGS_URL = 'https://www.imdb.com/title/{}/'
IMDB_JSON_REGEX = re.compile(
    r'<script type="application\/ld\+json">(.*?)<\/script>')


def get_details(imdb_id):
    # type: (Text) -> Dict
    """get the IMDB ratings details"""
    if not imdb_id:
        return {}
    votes, rating = _get_ratinginfo(imdb_id)
    return _assemble_imdb_result(votes, rating)


def _get_ratinginfo(imdb_id):
    # type: (Text) -> Tuple[Text, Text]
    """get the IMDB ratings details"""
    response = api_utils.load_info(IMDB_RATINGS_URL.format(
        imdb_id), default='', resp_type='text', verboselog=settings.VERBOSELOG)
    return _parse_imdb_result(response)


def _assemble_imdb_result(votes, rating):
    # type: (Text, Text) -> Dict
    """assemble to IMDB ratings into a Dict"""
    result = {}
    if votes and rating:
        result['ratings'] = {'imdb': {'votes': votes, 'rating': rating}}
    return result


def _parse_imdb_result(input_html):
    # type: (Text) -> Tuple[Text, Text]
    """parse the IMDB ratings from the JSON in the raw HTML"""
    match = re.search(IMDB_JSON_REGEX, input_html)
    if not match:
        return None, None
    imdb_json = json.loads(match.group(1))
    imdb_ratings = imdb_json.get("aggregateRating", {})
    rating = imdb_ratings.get("ratingValue", None)
    votes = imdb_ratings.get("ratingCount", None)
    return votes, rating
