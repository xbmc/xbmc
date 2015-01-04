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
    log('Version installed %s' %version_installed)
    ### Check to upgrade to newest available stable version
    # check on smaller major version. Smaller version than available always notify
    oldversion = False
    version_available = ''
    # check if installed major version is smaller than available major stable
    # here we don't care if running non stable
    if version_installed['major'] < int(versionlist_stable[0]['major']):
        version_available = versionlist_stable[0]
        oldversion = 'stable'
        log('Version available  %s' %versionlist_stable[0])
        log('You are running an older version')


    # check if installed major version is equal than available major stable
    # however also check on minor version and still don't care about non stable
    elif version_installed['major'] == int(versionlist_stable[0]['major']):
        if version_installed['minor'] < int(versionlist_stable[0]['minor']):
            version_available = versionlist_stable[0]
            oldversion = 'stable'
            log('Version available  %s' %versionlist_stable[0])
            log('You are running an older minor version')
        # check for <= minor !stable
        elif version_installed['tag'] != 'stable' and version_installed['minor'] <= int(versionlist_stable[0]['minor']):
            version_available = versionlist_stable[0]
            oldversion = True
            log('Version available  %s' %versionlist_stable[0])
            log('You are running an older non stable minor version')
        else:
            log('Version available  %s' %versionlist_stable[0])
            log('There is no newer stable available')
    
    # Already skipped a possible newer stable build. Let's continue with non stable builds.
    # Check also 'oldversion' hasn't been set to 'stable' or true by previous checks because if so,
    # those part need to be skipped
    
    #check for RC builds
    if not oldversion and version_installed['tag'] in ['releasecandidate']:
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0rc3 is newer than:  14.0rc1, 14.0b9, 14.0a15
        if version_installed['major'] <= int(versionlist_rc[0]['major']):
            if version_installed['minor'] <= int(versionlist_rc[0]['minor']):
                if version_installed.get('tagversion','') < versionlist_rc[0]['tagversion']:
                    version_available = versionlist_rc[0]
                    oldversion = True
                    log('Version available  %s' %versionlist_rc[0])
                    log('You are running an older RC version')
    # now check if installed !=rc
    elif not oldversion and version_installed['tag'] in ['beta','alpha','prealpha']:
        if version_installed['major'] <= int(versionlist_rc[0]['major']):
            if version_installed['minor'] <= int(versionlist_beta[0]['minor']):
                version_available = versionlist_rc[0]
                oldversion = True
                log('Version available  %s' %versionlist_rc[0])
                log('You are running an older non RC version')

    #check for beta builds
    if not oldversion and version_installed['tag'] == 'beta':
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0b3 is newer than:  14.0b1, 14.0a15
        if version_installed['major'] <= int(versionlist_beta[0]['major']):
            if version_installed['minor'] <= int(versionlist_beta[0]['minor']):
                if version_installed.get('tagversion','') < versionlist_beta[0]['tagversion']:
                    version_available = versionlist_beta[0]
                    oldversion = True
                    log('Version available  %s' %versionlist_beta[0])
                    log('You are running an older beta version')
    # now check if installed !=beta
    elif not oldversion and version_installed['tag'] in ['alpha','prealpha']:
        if version_installed['major'] <= int(versionlist_beta[0]['major']):
            if version_installed['minor'] <= int(versionlist_beta[0]['minor']):
                version_available = versionlist_beta[0]
                oldversion = True
                log('Version available  %s' %versionlist_beta[0])
                log('You are running an older non beta version')

    #check for alpha builds and older
    if not oldversion and version_installed['tag'] == 'alpha':
        # check if you are using a RC build lower than current available RC
        # then check if you are using a beta/alpha lower than current available RC
        # 14.0a3 is newer than: 14.0a1 or pre-alpha
        if version_installed['major'] <= int(versionlist_alpha[0]['major']):
            if version_installed['minor'] <= int(versionlist_alpha[0]['minor']):
                if version_installed.get('tagversion','') < versionlist_alpha[0]['tagversion']:
                    version_available = versionlist_alpha[0]
                    oldversion = True
                    log('Version available  %s' %versionlist_alpha[0])
                    log('You are running an older alpha version')
    # now check if installed !=alpha
    elif not oldversion and version_installed['tag'] in ['prealpha']:
        if version_installed['major'] <= int(versionlist_alpha[0]['major']):
            if version_installed['minor'] <= int(versionlist_alpha[0]['minor']):
                version_available = versionlist_alpha[0]
                oldversion = True
                log('Version available  %s' %versionlist_alpha[0])
                log('You are running an older non alpah version')     
    version_stable = versionlist_stable[0]
    return oldversion, version_installed, version_available, version_stable