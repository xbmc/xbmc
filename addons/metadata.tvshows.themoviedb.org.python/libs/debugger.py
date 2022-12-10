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
# pylint: disable=missing-docstring
#
# This is based on the metadata.tvmaze scrapper by Roman Miroshnychenko aka Roman V.M.

"""
Provides a context manager that writes extended debugging info
in the Kodi log on unhandled exceptions
"""
from __future__ import absolute_import, unicode_literals

import inspect
from contextlib import contextmanager
from platform import uname
from pprint import pformat

import xbmc

from .utils import logger

try:
    from typing import Text, Generator, Callable, Dict, Any  # pylint: disable=unused-import
except ImportError:
    pass


def _format_vars(variables):
    # type: (Dict[Text, Any]) -> Text
    """
    Format variables dictionary

    :param variables: variables dict
    :type variables: dict
    :return: formatted string with sorted ``var = val`` pairs
    :rtype: str
    """
    var_list = [(var, val) for var, val in variables.items()
                if not (var.startswith('__') or var.endswith('__'))]
    var_list.sort(key=lambda i: i[0])
    lines = []
    for var, val in var_list:
        lines.append('{0} = {1}'.format(var, pformat(val)))
    return '\n'.join(lines)


@contextmanager
def debug_exception(logger_func=logger.error):
    # type: (Callable[[Text], None]) -> Generator[None]
    """
    Diagnostic helper context manager

    It controls execution within its context and writes extended
    diagnostic info to the Kodi log if an unhandled exception
    happens within the context. The info includes the following items:

    - System info
    - Kodi version
    - Module path.
    - Code fragment where the exception has happened.
    - Global variables.
    - Local variables.

    After logging the diagnostic info the exception is re-raised.

    Example::

        with debug_exception():
            # Some risky code
            raise RuntimeError('Fatal error!')

    :param logger_func: logger function which must accept a single argument
        which is a log message.
    """
    try:
        yield
    except Exception as exc:
        frame_info = inspect.trace(5)[-1]
        logger_func(
            '*** Unhandled exception detected: {} {} ***'.format(type(exc), exc))
        logger_func('*** Start diagnostic info ***')
        logger_func('System info: {0}'.format(uname()))
        logger_func('OS info: {0}'.format(
            xbmc.getInfoLabel('System.OSVersionInfo')))
        logger_func('Kodi version: {0}'.format(
            xbmc.getInfoLabel('System.BuildVersion')))
        logger_func('File: {0}'.format(frame_info[1]))
        context = ''
        if frame_info[4] is not None:
            for i, line in enumerate(frame_info[4], frame_info[2] - frame_info[5]):
                if i == frame_info[2]:
                    context += '{0}:>{1}'.format(str(i).rjust(5), line)
                else:
                    context += '{0}: {1}'.format(str(i).rjust(5), line)
        logger_func('Code context:\n' + context)
        logger_func('Global variables:\n' +
                    _format_vars(frame_info[0].f_globals))
        logger_func('Local variables:\n' +
                    _format_vars(frame_info[0].f_locals))
        logger_func('**** End diagnostic info ****')
        raise exc
