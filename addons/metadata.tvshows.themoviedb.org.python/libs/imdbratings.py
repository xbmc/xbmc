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
from . import api_utils
from . import settings

IMDB_RATINGS_URL = 'https://www.imdb.com/title/{}/'
IMDB_RATING_REGEX = re.compile(r'itemprop="ratingValue".*?>.*?([\d.]+).*?<')
IMDB_VOTES_REGEX = re.compile(r'itemprop="ratingCount".*?>.*?([\d,]+).*?<')


def get_details(imdb_id):
    if not imdb_id:
        return {}
    votes, rating = _get_ratinginfo(imdb_id)
    return _assemble_imdb_result(votes, rating)

def _get_ratinginfo(imdb_id):
    response = api_utils.load_info(IMDB_RATINGS_URL.format(imdb_id), default = '', resp_type='text', verboselog=settings.VERBOSELOG)
    return _parse_imdb_result(response)

def _assemble_imdb_result(votes, rating):
    result = {}
    if votes and rating:
        result['ratings'] = {'imdb': {'votes': votes, 'rating': rating}}
    return result

def _parse_imdb_result(input_html):
    rating = _parse_imdb_rating(input_html)
    votes = _parse_imdb_votes(input_html)
    return votes, rating

def _parse_imdb_rating(input_html):
    match = re.search(IMDB_RATING_REGEX, input_html)
    if (match):
        return float(match.group(1))
    return None

def _parse_imdb_votes(input_html):
    match = re.search(IMDB_VOTES_REGEX, input_html)
    if (match):
        return int(match.group(1).replace(',', ''))
    return None
