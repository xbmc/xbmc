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
    ### Check to upgrade to newest available stable version
    # check on smaller major version. Smaller version than available always notify
    oldversion = False
    msg = ''
    if version_installed['major'] < int(versionlist_stable[0]['major']):
        msg = 32003
        oldversion = True
        log("Version available  %s" %versionlist_stable[0])

    # check on same major version installed and available
    elif version_installed['major'] == int(versionlist_stable[0]['major']):
        # check on smaller minor version
        if version_installed['minor'] < int(versionlist_stable[0]['minor']):
            msg = 32003
            oldversion = True
            log("Version available  %s" %versionlist_stable[0])
        # check if not installed a stable so always notify
        elif version_installed['minor'] == int(versionlist_stable[0]['minor']) and version_installed['tag'] != "stable":
            msg = 32008
            oldversion = True
            log("Version available  %s" %versionlist_stable[0])
        else:
            log("Last available stable installed")

    ### Check to upgrade to newest available RC version if not installed stable
    ## Check also oldversion hasn't been set true by previous check because if so this need to be skipped
    if not oldversion and version_installed['tag'] != "stable":
        if 'revision' in version_installed.keys():
            # only check on equal or lower major because newer installed beta/alpha/prealpha version will be higher
            if versionlist_rc and version_installed['major'] <= int(versionlist_rc[0]['major']):
                if version_installed['revision'] <= versionlist_rc[0]['revision']:
                    msg = 32004
                    oldversion = True
                    log("Version available  %s" %versionlist_rc[0])

            # exclude if installed RC on checking for newer beta
            if not oldversion and versionlist_beta and version_installed['tag'] not in ["releasecandidate"]:
                if version_installed['major'] <= int(versionlist_beta[0]['major']):
                    if version_installed['revision'] < versionlist_beta[0]['revision']:
                        msg = 32005
                        oldversion = True
                        log("Version available  %s" %versionlist_beta[0])
        
            # exclude if installed RC or beta on checking for newer alpha
            if not oldversion and versionlist_alpha and version_installed['tag'] not in ["releasecandidate", "beta"]:
                if version_installed['major'] <= int(versionlist_alpha[0]['major']):
                    if version_installed['revision'] < versionlist_alpha[0]['revision']:
                        msg = 32006
                        oldversion = True
                        log("Version available  %s" %versionlist_alpha[0])

            # exclude if installed RC, beta or alpha on checking for newer prealpha
            if not oldversion and versionlist_prealpha and version_installed['tag'] not in ["releasecandidate", "beta", "alpha"]:
                if version_installed['major'] <= int(versionlist_prealpha[0]['major']):
                    if  version_installed['revision'] < versionlist_prealpha[0]['revision']:
                        msg = 32007
                        oldversion = True
                        log("Version available  %s" %versionlist_prealpha[0])

        log("Nothing to see here, move along. Running a latest non stable release")
        # Nothing to see here, move along
    else:
        log("Nothing to see here, move along. Running a stable release")
        # Nothing to see here, move along
        pass
    return oldversion, msg