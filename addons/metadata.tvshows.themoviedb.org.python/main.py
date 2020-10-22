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
# Code attribution:
# core based on metadata.tvmaze scrapper by Roman Miroshnychenko aka Roman V.M.
# IMDb ratings based on code in metadata.themoviedb.org.python by Team Kodi
# pylint: disable=missing-docstring

from __future__ import absolute_import

import sys

from libs.actions import router
from libs.debugger import debug_exception

if __name__ == '__main__':
    with debug_exception():
        router(sys.argv[2][1:])
