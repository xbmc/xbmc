//
//  PlexManualServerManager.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#include "PlexManualServerManager.h"
#include "PlexServerManager.h"
#include "settings/GUISettings.h"
#include "PlexJobs.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "PlexApplication.h"

#include <boost/lexical_cast.hpp>


void CPlexManualServerManager::checkManualServersAsync()
{
  CSingleLock lk(m_manualServerLock);

  m_waitingForThreads = 1;
  m_manualServers.clear();

  CPlexServerPtr localServer = CPlexServerPtr(new CPlexServer(CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, "127.0.0.1", 32400, "http"))));
  CJobManager::GetInstance().AddJob(new CPlexHTTPFetchJob(localServer->BuildURL("/"), localServer), this);

  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    CStdString address = g_guiSettings.GetString("plexmediaserver.address");
    int port = boost::lexical_cast<int>(g_guiSettings.GetString("plexmediaserver.port"));

    m_waitingForThreads ++;

    CPlexServerPtr manualServer = CPlexServerPtr(new CPlexServer(CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, address, port, "http"))));
    CJobManager::GetInstance().AddJob(new CPlexHTTPFetchJob(manualServer->BuildURL("/"), manualServer), this);
  }

  g_plexApplication.timer->SetTimeout(1 * 60 * 1000, this);
}

void CPlexManualServerManager::OnTimeout()
{
  checkManualServersAsync();
  g_plexApplication.timer->SetTimeout(1 * 60 * 1000, this);
}

void CPlexManualServerManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexHTTPFetchJob *httpJob = static_cast<CPlexHTTPFetchJob*>(job);
  
  if (success)
  {
    std::string uuid, name, version;
    
    CXBMCTinyXML doc;
    doc.Parse(httpJob->m_data);
    if (!doc.Error() && doc.RootElement())
    {
      TiXmlElement *root = doc.RootElement();
      
      if (root->QueryStringAttribute("machineIdentifier", &uuid) == TIXML_SUCCESS &&
          root->QueryStringAttribute("friendlyName", &name) == TIXML_SUCCESS &&
          root->QueryStringAttribute("version", &version) == TIXML_SUCCESS)
      {
        CPlexServerPtr server = httpJob->m_server;
        server->SetUUID(uuid);
        server->SetName(name);
        server->SetVersion(version);
        server->SetOwned(true);

        server->CollectDataFromRoot(httpJob->m_data);

        CLog::Log(LOGDEBUG, "CPlexManualServerManager::OnJobComplete found manually added server %s", server->GetName().c_str());

        g_plexApplication.serverManager->UpdateFromDiscovery(server);

        CSingleLock lk(m_manualServerLock);
        m_manualServers.push_back(server);
      }
    }
  }
  else
    CLog::Log(LOGWARNING, "CPlexManualServerManager::OnJobComplete failed to find a server on %s", httpJob->m_url.Get().c_str());

  CSingleLock lk(m_manualServerLock);
  if (-- m_waitingForThreads == 0)
  {
    CLog::Log(LOGDEBUG, "CPlexManualServerManager::OnJobComplete all manual server checks done, updating serverManager");
    g_plexApplication.serverManager->UpdateFromConnectionType(m_manualServers, CPlexConnection::CONNECTION_MANUAL);
  }
}
