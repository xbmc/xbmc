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
#include "PlexApplication.h"
#include "GUISettings.h"

#define DEFAULT_PORT 32400

CMyPlexManager::EMyPlexError CMyPlexScanner::DoScan()
{
  CPlexServerPtr myplex = g_plexApplication.serverManager->FindByUUID("myplex");
  CURL url = myplex->BuildPlexURL("pms/servers");
  url.SetOption("includeLite", "1");

  XFILE::CPlexDirectory dir;
  CFileItemList list;

  if (!dir.GetDirectory(url.Get(), list))
  {
    if (dir.GetHTTPResponseCode() == 401)
    {
      CLog::Log(LOGERROR, "CMyPlexScanner::DoScan not authorized from myPlex");
      return CMyPlexManager::ERROR_WRONG_CREDS;
    }
    return CMyPlexManager::ERROR_NOERROR;
  }

  PlexServerList serverList;
  for (int i = 0; i < list.Size(); i ++)
  {
    CFileItemPtr serverItem = list.Get(i);
    if (serverItem && serverItem->GetPlexDirectoryType() == PLEX_DIR_TYPE_SERVER)
    {
      bool synced = serverItem->GetProperty("synced").asBoolean();

      if (synced && g_guiSettings.GetBool("myplex.hidecloudsync"))
      {
        CLog::Log(LOGDEBUG, "CMyPlexScanner::DoScan hiding cloudsync server");
        continue;
      }

      CStdString uuid = serverItem->GetProperty("machineIdentifier").asString();
      CStdString name = serverItem->GetProperty("name").asString();
      bool owned = serverItem->GetProperty("owned").asBoolean();
      bool home = serverItem->GetProperty("home").asBoolean(false);

      if (uuid.empty() || name.empty())
        continue;

      CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, name, owned));

      if (serverItem->HasProperty("sourceTitle"))
        server->SetOwner(serverItem->GetProperty("sourceTitle").asString());
      server->SetHome(home);

      CStdString address = serverItem->GetProperty("address").asString();
      CStdString token = serverItem->GetProperty("accessToken").asString();
      CStdString localAddresses = serverItem->GetProperty("localAddresses").asString();
      CStdString schema = serverItem->GetProperty("scheme").asString();
      int port = serverItem->GetProperty("port").asInteger();

      if (token.empty())
        continue;

      if (!address.empty())
      {
        CPlexConnectionPtr connection = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX, address, port, schema, token));
        server->AddConnection(connection);
      }

      /* only add localConnections for non-shared servers */
      if ((owned || home) && !localAddresses.empty())
      {
        CStdStringArray addressList = StringUtils::SplitString(localAddresses, ",", 0);
        BOOST_FOREACH(CStdString laddress, addressList)
        {
          CPlexConnectionPtr lconn = CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MYPLEX, laddress, DEFAULT_PORT, schema, token));
          server->AddConnection(lconn);
        }
      }

      serverList.push_back(server);
    }
  }
  
  BOOST_FOREACH(const CPlexServerPtr& server, serverList)
  {
    CLog::Log(LOGDEBUG, "CMyPlexScanner::DoScan server found: %s (%s) (owned: %s, home: %s)", server->GetName().c_str(), server->GetUUID().c_str(), server->GetOwned() ? "YES" : "NO", server->GetHome() ? "YES" : "NO");
    std::vector<CPlexConnectionPtr> connections;
    server->GetConnections(connections);
    BOOST_FOREACH(const CPlexConnectionPtr& conn, connections)
      CLog::Log(LOGDEBUG, "CMyPlexScanner::DoScan              - %s (isLocal: %s)", conn->GetAddress().Get().c_str(), conn->IsLocal() ? "YES" : "NO");
  }

  g_plexApplication.serverManager->UpdateFromConnectionType(serverList, CPlexConnection::CONNECTION_MYPLEX);

  return CMyPlexManager::ERROR_NOERROR;
}
