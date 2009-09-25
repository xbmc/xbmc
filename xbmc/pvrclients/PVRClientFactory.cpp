/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "StringUtils.h"
#include "utils/log.h"
#include "FileItem.h"
#include "PVRClientFactory.h"
#include "DllPVRClient.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "utils/Addon.h"

using namespace ADDON;
using namespace XFILE;

CPVRClientFactory::CPVRClientFactory()
{

}

CPVRClientFactory::~CPVRClientFactory()
{

}

CPVRClient* CPVRClientFactory::LoadPVRClient(const CAddon& addon, DWORD clientID, IPVRClientCallback *pvrCB) const
{
#ifdef HAS_PVRCLIENTS
  // add the library name readed from info.xml to the addon's path
  CStdString strFileName;
  if (addon.m_guid_parent.IsEmpty())
  {
    strFileName = addon.m_strPath + addon.m_strLibName;
  }
  else
  {
    CStdString extension = CUtil::GetExtension(addon.m_strLibName);
    strFileName = "special://temp/" + addon.m_strLibName;
    CUtil::RemoveExtension(strFileName);
    strFileName += "-" + addon.m_guid + extension;

    if (!CFile::Exists(strFileName))
      CFile::Cache(addon.m_strPath + addon.m_strLibName, strFileName);

    CLog::Log(LOGNOTICE, "Loaded virtual child addon %s", strFileName.c_str());
  }

  // load client
  DllPVRClient* pDll = new DllPVRClient;
  pDll->SetFile(strFileName);

  pDll->EnableDelayedUnload(true);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  // Create library address table
  struct PVRClient* pClient = (struct PVRClient*)malloc(sizeof(struct PVRClient));
  ZeroMemory(pClient, sizeof(struct PVRClient));
  pDll->GetAddon(pClient);

  // and pass it to a new instance of CPVRClient() which will handle the client
  CPVRClient *client(new CPVRClient(clientID, pClient, pDll, addon, pvrCB));
  // Initialize client and perform all steps to become it running
  client->Init();

  return client;
#else
  return NULL;
#endif
}
