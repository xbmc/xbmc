# -*- coding: utf-8 -*-
#
#     Copyright (C) 2013 Team-XBMC
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
#

from lib.common import log

def compare_version(version_installed, versionlist):
    # Create seperate version lists
    versionlist_stable = versionlist['releases']['stable']
    versionlist_rc = versionlist['releases']['releasecandidate']
    versionlist_beta = versionlist['releases']['beta']
    versionlist_alpha = versionlist['releases']['alpha']
    versionlist_prealpha = versionlist['releases']['prealpha'] 
    log("Version installed %s" %version_installed)
    ### Check to upgrade to newest available stable version
    # check on smaller major version. Smaller version than available always notify
    oldversion = False
    msg = ''

    # check if current major version is smaller than available major stable
    # here we don't care if running non stable
    if version_installed['major'] < int(versionlist_stable[0]['major']):
        msg = 32003
        oldversion = "stable"
        log("Version available  %s" %versionlist_stable[0])
        log("You are running an older version")


    # check if current major version is equal than available major stable
    # however also check on minor version and still don't care about non stable
    elif version_installed['major'] == int(versionlist_stable[0]['major']):
        if version_installed['minor'] < int(versionlist_stable[0]['minor']):
            msg = 32003
            oldversion = "stable"
            log("Version available  %s" %versionlist_stable[0])
            log("You are running an older minor version")
        # check for <= minor !stable
        elif version_installed['tag'] != "stable" and version_installed['minor'] <= int(versionlist_stable[0]['minor']):
            msg = 32003
            oldversion = True
            log("Version available  %s" %versionlist_stable[0])
            log("You are running an older non stable minor version")
        else:
            log("Version available  %s" %versionlist_stable[0])
            log("There is no newer stable available")

    ## Check to upgrade to newest available RC version if not installed stable
    ## Check also oldversion hasn't been set true by previous check because if so this need to be skipped
    elif version_installed['tag'] != "stable":
        # check if you are using a RC lower than current RC
        # then check if you are using a alpha/beta lower than current RC
        if version_installed['major'] <= int(versionlist_rc[0]['major']):
            if version_installed['tag'] in ["releasecandidate"]:
                if version_installed['revision'] < versionlist_rc[0]['revision']:
                    msg = 32004
                    oldversion = True
                    log("Version available  %s" %versionlist_rc[0])
                    log("You are running an older RC version")
                elif version_installed['revision'] < versionlist_rc[0]['revision']:
                    msg = 32004
                    oldversion = True
                    log("Version available  %s" %versionlist_rc[0])
                    log("You are running an older non RC version")
                else:
                    log("Version available  %s" %versionlist_rc[0])
                    log("You are running newest RC version")

        # exclude if you are running an RC
        # check if you are using a beta lower than current beta
        # then check if you are using a alpha/beta lower than current RC
        if not oldversion and version_installed['tag'] not in ["releasecandidate"] and version_installed['major'] <= int(versionlist_beta[0]['major']):
            if version_installed['tag'] in ["beta"]:
                if version_installed['revision'] < versionlist_beta[0]['revision']:
                    msg = 32005
                    oldversion = True
                    log("Version available  %s" %versionlist_beta[0])
                    log("You are running an older beta version")
                elif version_installed['revision'] < versionlist_beta[0]['revision']:
                    msg = 32005
                    oldversion = True
                    log("Version available  %s" %versionlist_beta[0])
                    log("You are running an older non beta version")
                else:
                    log("Version available  %s" %versionlist_beta[0])
                    log("You are running newest beta version")

        # exclude if you are running an RC/beta
        # check if you are using a beta lower than current beta
        # then check if you are using a alpha/beta lower than current RC
        if not oldversion and version_installed['tag'] in ["prealpha"] and version_installed['major'] <= int(versionlist_prealpha[0]['major']):
            if version_installed['revision'] < versionlist_prealpha[0]['revision']:
                msg = 32006
                oldversion = True
                log("Version available  %s" %versionlist_prealpha[0])
                log("You are running an older alpha version")
            else:
                log("Version available  %s" %versionlist_prealpha[0])
                log("You are running newest alpha version")

    return oldversion, msg