/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#include "stdafx.h"
#include "Addon.h"
#include "PVRClientFactory.h"
#include "DllPVRClient.h"
#include "Util.h"
#include "FileSystem/File.h"

using namespace XFILE;
using namespace ADDON;

CPVRClient* CPVRClientFactory::LoadPVRClient(const CStdString& path, const CStdString& library, const CStdString& name, DWORD clientID, ADDON::IAddonCallback *addonCB, IPVRClientCallback *pvrCB)
{
  // add the .pvr extension to the addon's path
  CStdString strFileName = path + library;

  //_T(strFileName);

  // load client
  DllPVRClient* pDll = new DllPVRClient;
  pDll->SetFile(strFileName);

  pDll->EnableDelayedUnload(true);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  struct PVRClient* pClient = (struct PVRClient*)malloc(sizeof(struct PVRClient));
  ZeroMemory(pClient, sizeof(struct PVRClient));
  pDll->GetAddon(pClient);

  // and pass it to a new instance of CPVRClient() which will handle the client
  CPVRClient *client(new CPVRClient(clientID, pClient, pDll, name, addonCB, pvrCB));
  client->Init();

  return client;
}
