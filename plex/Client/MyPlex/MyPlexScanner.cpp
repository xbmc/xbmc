//
//  MyPlexScanner.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-12.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "MyPlexScanner.h"
#include "Client/PlexServerManager.h"
#include "XBMCTinyXML.h"
#include "FileSystem/PlexFile.h"
#include "FileSystem/PlexDirectory.h"
#include "utils/StringUtils.h"

#define DEFAULT_PORT 32400

bool CMyPlexScanner::DoScan()
{
  CPlexServerPtr myplex = g_plexServerManager.FindByUUID("myplex");
  CURL url = myplex->BuildPlexURL("pms/servers");

  XFILE::CPlexDirectory dir;
  CFileItemList list;

  if (!dir.GetDirectory(url.Get(), list))
  {
    if (dir.GetHTTPResponseCode() == 401)
    {
      CLog::Log(LOGERROR, "CMyPlexScanner::DoScan not authorized from myPlex");
      return false;
    }
    return true;
  }

  PlexServerList serverList;
  for (int i = 0; i < list.Size(); i ++)
  {
    CFileItemPtr serverItem = list.Get(i);
    if (serverItem && serverItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_SERVER)
    {
      CStdString uuid = serverItem->GetProperty("machineIdentifier").asString();
      CStdString name = serverItem->GetProperty("name").asString();
      bool owned = serverItem->GetProperty("owned").asBoolean();

      if (uuid.empty() || name.empty())
        continue;

      CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, name, owned));

      if (serverItem->HasProperty("sourceTitle"))
        server->SetOwner(serverItem->GetProperty("sourceTitle").asString());

      CStdString address = serverItem->GetProperty("address").asString();
      CStdString token = serverItem->GetProperty("accessToken").asString();
      CStdString localAddresses = serverItem->GetProperty("localAddresses").asString();
      int port = serverItem->GetProperty("port").asInteger();

      if (address.empty() || token.empty())
        continue;

      CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX, address, port, token));
      server->AddConnection(connection);

      if (!localAddresses.empty())
      {
        CStdStringArray addressList = StringUtils::SplitString(localAddresses, ",", 0);
        BOOST_FOREACH(CStdString laddress, addressList)
        {
          CPlexConnectionPtr lconn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX, laddress, DEFAULT_PORT, token));
          server->AddConnection(lconn);
        }
      }

      g_plexServerManager.UpdateFromDiscovery(server);
      serverList.push_back(server);
    }
  }

  g_plexServerManager.UpdateFromConnectionType(serverList, CPlexConnection::CONNECTION_MYPLEX);

  /* now we need to store away the thumbnails */
  CURL sectionsURL = myplex->BuildPlexURL("pms/system/library/sections");

  CFileItemList sectionList;
  if (!dir.GetDirectory(sectionsURL.Get(), sectionList))
  {
    return true;
  }

  CMyPlexSectionMap sectionMap;

  for (int i = 0; i < sectionList.Size(); i ++)
  {
    CFileItemPtr section = sectionList.Get(i);
    CStdString serverUUID = section->GetProperty("machineIdentifier").asString();

    CLog::Log(LOGDEBUG, "CMyPlexScanner::DoScan found section %s for server %s", section->GetProperty("path").asString().c_str(), serverUUID.c_str());

    if (sectionMap.find(serverUUID) != sectionMap.end())
    {
      sectionMap[serverUUID]->Add(section);
    }
    else
    {
      sectionMap[serverUUID] = CFileItemListPtr(new CFileItemList);
      sectionMap[serverUUID]->Add(section);
    }
  }

  g_myplexManager.SetSectionMap(sectionMap);

  return true;
}
