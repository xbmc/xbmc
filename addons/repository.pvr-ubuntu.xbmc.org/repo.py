#/*
# *      Copyright (C) 2010 Arne Morten Kvarving
# *
# *
# *  This Program is free software; you can redistribute it and/or modify
# *  it under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 2, or (at your option)
# *  any later version.
# *
# *  This Program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; see the file COPYING.  If not, write to
# *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
# *  http://www.gnu.org/copyleft/gpl.html
# *
# */

import xbmc, xbmcaddon, xbmcplugin
import sys
import os
from aptdaemon import client

def get_params():
  param=[]
  paramstring=sys.argv[2]
  if len(paramstring)>=2:
    params=sys.argv[2]
    cleanedparams=params.replace('?','')
    if (params[len(params)-1]=='/'):
      params=params[0:len(params)-2]
    pairsofparams=cleanedparams.split('&')
    param={}
    for i in range(len(pairsofparams)):
      splitparams={}
      splitparams=pairsofparams[i].split('=')
      if (len(splitparams))==2:
        param[splitparams[0]]=splitparams[1]
                                
  return param

addon = xbmcaddon.Addon(id='repository.pvr-ubuntu.xbmc.org')
params=get_params()
try:
  action = params["action"]
except:
  action = "";
  pass
try:
  package = params["package"]
except:
  package = "";
  pass
try:
  version = params["version"]
except:
  version= "";
  pass

if action == "update":
  client = client.AptClient()
#  if addon.getSetting('added') is not 'true':
#    add = client.add_repository('deb','http://ppa.launchpad.net/team-xbmc/ppa/ubuntu','quantal','main', wait=True)
#    addon.setSetting('added','true')

  source = os.path.join(xbmc.translatePath(addon.getAddonInfo('path')),'sources.list')
  update = client.update_cache(sources_list=source, wait=True)

if action == "install":
  myclient = client.AptClient()
  if len(version):
    install = myclient.install_packages(['%s=%s' %(package, version)],wait=True)
  else:
    install = myclient.install_packages(['%s' %(package)],wait=True)

if action == "uninstall":
  myclient = client.AptClient()
  install = myclient.remove_packages(['%s' %(package)],wait=True)

xbmcplugin.endOfDirectory(int(sys.argv[1]))
