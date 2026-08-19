# SPDX-License-Identifier: GPL-3.0-or-later

"""Logging gated by the verbose_log setting."""

import xbmc

from lib.config import ADDON

_PREFIX = ADDON.getAddonInfo('id')
_verbose = False


def init(verbose):
    global _verbose
    _verbose = verbose


def info(msg):
    xbmc.log('[{}] {}'.format(_PREFIX, msg), xbmc.LOGINFO)


def debug(msg):
    if _verbose:
        xbmc.log('[{}] {}'.format(_PREFIX, msg), xbmc.LOGDEBUG)


def error(msg):
    xbmc.log('[{}] {}'.format(_PREFIX, msg), xbmc.LOGERROR)
