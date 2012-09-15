/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "WinNetworkManager.h"
#ifdef _WIN32
#include "WinConnection.h"

CWinNetworkManager::CWinNetworkManager()
{
}

CWinNetworkManager::~CWinNetworkManager()
{
}

bool CWinNetworkManager::Connect()
{
  return false;
}

bool CWinNetworkManager::CanManageConnections()
{
  return false;
}

ConnectionList CWinNetworkManager::GetConnections()
{
  ConnectionList connections;

  PIP_ADAPTER_INFO adapterInfo;
  PIP_ADAPTER_INFO adapter = NULL;

  ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);

  adapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
  if (adapterInfo == NULL) 
    return;

  if (GetAdaptersInfo(adapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
  {
    free(adapterInfo);
    adapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
    if (adapterInfo == NULL) 
    {
      OutputDebugString("Error allocating memory needed to call GetAdaptersinfo\n");
      return;
    }
  }

  if ((GetAdaptersInfo(adapterInfo, &ulOutBufLen)) == NO_ERROR) 
  {
    adapter = adapterInfo;
    while (adapter) 
    {
      connections.push_back(CConnectionPtr(new CWinConnection(*adapter)));

      adapter = adapter->Next;
    }
  }

  free(adapterInfo);

  return connections;
}

bool CWinNetworkManager::PumpNetworkEvents(INetworkEventsCallback *callback)
{
  return false;
}
#endif
