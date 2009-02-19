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
#include "PVRClientFactory.h"
#include "DllPVRClient.h"
#include "Util.h"
#include "FileSystem/File.h"

using namespace XFILE;

IPVRClient* CPVRClientFactory::LoadPVRClient(const CStdString& strClient, DWORD clientID, IPVRClientCallback *cb)
{
  // strip of the path & extension to get the name of the client
  // like vdr or mythtv
  CStdString strFileName = strClient;
  CStdString strName = CUtil::GetFileName(strClient);

  // if it's a relative path or just a name, convert to absolute path
  if ( strFileName[1] != ':' && strFileName[0] != '/' )
  {
    // first check home
    strFileName.Format("special://home/pvrclients/%s", strName.c_str() );

    // if not found, use system
    if ( ! CFile::Exists( strFileName ) )
      strFileName.Format("special://xbmc/pvrclients/%s", strName.c_str() );
  }
  strName = strName.Left(strName.ReverseFind('.'));

#ifdef HAS_PVRCLIENTS
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
  pDll->GetPlugin(pClient);

  // and pass it to a new instance of CPVRClient() which will handle the client
  CPVRClient *client(new CPVRClient(clientID, pClient, pDll, strName, cb));
  client->Init();

  return client;
#else
  return NULL;
#endif
}
