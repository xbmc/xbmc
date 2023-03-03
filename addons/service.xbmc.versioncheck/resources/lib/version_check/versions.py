# -*- coding: utf-8 -*-

"""

    Copyright (C) 2013-2014 Team-XBMC
    Copyright (C) 2014-2019 Team Kodi

    This file is part of service.xbmc.versioncheck

    SPDX-License-Identifier: GPL-3.0-or-later
    See LICENSES/GPL-3.0-or-later.txt for more information.

"""

from .common import log


def compare_version(version_installed, version_list):
    """ Compare the installed version against the provided version list

    :param version_installed: currently installed version
    :type version_installed: dict
    :param version_list: provided versions to compare against
    :type version_list: dict
    :return: old, current, available, and stable versions
    :rtype: bool / 'stable', dict, dict, dict
    """
    # Create separate version lists
    version_list_stable = version_list['releases']['stable']
    version_list_rc = version_list['releases']['releasecandidate']
    version_list_beta = version_list['releases']['beta']
    version_list_alpha = version_list['releases']['alpha']
    # version_list_prealpha = version_list['releases']['prealpha']

    stable_version = version_list_stable[0]
    rc_version = version_list_rc[0]
    beta_version = version_list_beta[0]
    alpha_version = version_list_alpha[0]

    log('Version installed %s' % version_installed)

    # Check to upgrade to newest available stable version
    # check on smaller major version. Smaller version than available always notify

    # check for stable versions
    old_version, version_available = _check_for_stable_version(version_installed, stable_version)

    if not old_version:
        # Already skipped a possible newer stable build. Let's continue with non stable builds.
        # Check also 'old version' hasn't been set to 'stable' or true by previous checks because
        # if so, those part need to be skipped
        old_version, version_available = _check_for_rc_version(version_installed,
                                                               rc_version, beta_version)

    if not old_version:
        # check for beta builds
        old_version, version_available = _check_for_beta_version(version_installed, beta_version)

    if not old_version:
        # check for alpha builds
        old_version, version_available = _check_for_alpha_version(version_installed, alpha_version)

    return old_version, version_installed, version_available, stable_version


def _check_for_stable_version(version_installed, stable_version):
    """ Compare the installed version against the latest stable version

    :param version_installed: currently installed version
    :type version_installed: dict
    :param stable_version: latest stable version
    :type stable_version: dict
    :return: whether using an old version, and available version if newer stable version available
    :rtype: bool / 'stable', dict
    """
    # check if installed major version is smaller than available major stable
    # here we don't care if running non stable
    old_version = False
    version_available = {}

    if version_installed['major'] < int(stable_version['major']):
        version_available = stable_version
        old_version = 'stable'
        log('Version available  %s' % stable_version)
        log('You are running an older version')

    # check if installed major version is equal than available major stable
    # however also check on minor version and still don't care about non stable
    elif version_installed['major'] == int(stable_version['major']):
        if version_installed['minor'] < int(stable_version['minor']):
            version_available = stable_version
            old_version = 'stable'
            log('Version available  %s' % stable_version)
            log('You are running an older minor version')
        # check for <= minor !stable
        elif version_installed['tag'] != 'stable' and \
                version_installed['minor'] <= int(stable_version['minor']):
            version_available = stable_version
            old_version = True
            log('Version available  %s' % stable_version)
            log('You are running an older non stable minor version')
        else:
            log('Version available  %s' % stable_version)
            log('There is no newer stable available')

    return old_version, version_available


def _check_for_rc_version(version_installed, rc_version, beta_version):
    """ Compare the installed version against the latest RC version

    :param version_installed: currently installed version
    :type version_installed: dict
    :param rc_version: latest rc version
    :type rc_version: dict
    :param beta_version: latest beta version
    :type beta_version: dict
    :return: whether using an old version, and available version if newer rc version available
    :rtype: bool, dict
    """
    old_version = False
    version_available = {}
    # check for RC builds
    if version_installed['tag'] in ['releasecandidate']:
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0rc3 is newer than:  14.0rc1, 14.0b9, 14.0a15
        if version_installed['major'] <= int(rc_version['major']):
            if version_installed['minor'] <= int(rc_version['minor']):
                if version_installed.get('tagversion', '') < rc_version['tagversion']:
                    version_available = rc_version
                    old_version = True
                    log('Version available  %s' % rc_version)
                    log('You are running an older RC version')
    # now check if installed !=rc
    elif version_installed['tag'] in ['beta', 'alpha', 'prealpha']:
        if version_installed['major'] <= int(rc_version['major']):
            if version_installed['minor'] <= int(beta_version['minor']):
                version_available = rc_version
                old_version = True
                log('Version available  %s' % rc_version)
                log('You are running an older non RC version')

    return old_version, version_available


def _check_for_beta_version(version_installed, beta_version):
    """ Compare the installed version against the latest beta version

    :param version_installed: currently installed version
    :type version_installed: dict
    :param beta_version: latest beta version
    :type beta_version: dict
    :return: whether using an old version, and available version if newer beta version available
    :rtype: bool, dict
    """
    old_version = False
    version_available = {}
    # check for beta builds
    if not old_version and version_installed['tag'] == 'beta':
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0b3 is newer than:  14.0b1, 14.0a15
        if version_installed['major'] <= int(beta_version['major']):
            if version_installed['minor'] <= int(beta_version['minor']):
                if version_installed.get('tagversion', '') < beta_version['tagversion']:
                    version_available = beta_version
                    old_version = True
                    log('Version available  %s' % beta_version)
                    log('You are running an older beta version')
    # now check if installed !=beta
    elif not old_version and version_installed['tag'] in ['alpha', 'prealpha']:
        if version_installed['major'] <= int(beta_version['major']):
            if version_installed['minor'] <= int(beta_version['minor']):
                version_available = beta_version
                old_version = True
                log('Version available  %s' % beta_version)
                log('You are running an older non beta version')

    return old_version, version_available


def _check_for_alpha_version(version_installed, alpha_version):
    """ Compare the installed version against the latest alpha version

    :param version_installed: currently installed version
    :type version_installed: dict
    :param alpha_version: latest alpha version
    :type alpha_version: dict
    :return: whether using an old version, and available version if newer alpha version available
    :rtype: bool, dict
    """
    old_version = False
    version_available = {}
    # check for alpha builds and older
    if version_installed['tag'] == 'alpha':
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0a3 is newer than: 14.0a1 or pre-alpha
        if version_installed['major'] <= int(alpha_version['major']):
            if version_installed['minor'] <= int(alpha_version['minor']):
                if version_installed.get('tagversion', '') < alpha_version['tagversion']:
                    version_available = alpha_version
                    old_version = True
                    log('Version available  %s' % alpha_version)
                    log('You are running an older alpha version')
    # now check if installed !=alpha
    elif version_installed['tag'] in ['prealpha']:
        if version_installed['major'] <= int(alpha_version['major']):
            if version_installed['minor'] <= int(alpha_version['minor']):
                version_available = alpha_version
                old_version = True
                log('Version available  %s' % alpha_version)
                log('You are running an older non alpha version')

    return old_version, version_available
