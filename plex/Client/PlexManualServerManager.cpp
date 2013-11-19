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

void CPlexManualServerManager::checkLocalhost()
{
  CURL url;
  url.SetProtocol("http");
  url.SetHostName("127.0.0.1");
  url.SetPort(32400);

  CJobManager::GetInstance().AddJob(new CPlexHTTPFetchJob(url), this);
}

void CPlexManualServerManager::checkManualServersAsync()
{
  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    CStdString address = g_guiSettings.GetString("plexmediaserver.address");
    int port = boost::lexical_cast<int>(g_guiSettings.GetString("plexmediaserver.port"));
    
    CURL url;
    url.SetProtocol("http");
    url.SetHostName(address);
    url.SetPort(port);
    
    CJobManager::GetInstance().AddJob(new CPlexHTTPFetchJob(url), this);
  }
  else
  {
    CSingleLock lk(m_serverLock);
    if (m_serverMap.find(m_manualServerUUID) != m_serverMap.end())
    {
      m_serverMap.erase(m_manualServerUUID);
      m_manualServerUUID.clear();
    }
    updateServerManager();
  }

  checkLocalhost();
  g_plexApplication.timer.SetTimeout(1 * 60 * 1000, this);
}

void CPlexManualServerManager::OnTimeout()
{
  checkManualServersAsync();
  g_plexApplication.timer.SetTimeout(1 * 60 * 1000, this);
}

void CPlexManualServerManager::updateServerManager()
{
  CSingleLock lk(m_serverLock);
  PlexServerList list;
  BOOST_FOREACH(PlexServerPair p, m_serverMap)
      list.push_back(p.second);

  g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MANUAL);
}

void CPlexManualServerManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexHTTPFetchJob *httpJob = static_cast<CPlexHTTPFetchJob*>(job);
  
  if (success)
  {
    std::string uuid, name;
    
    CXBMCTinyXML doc;
    doc.Parse(httpJob->m_data);
    if (!doc.Error() && doc.RootElement())
    {
      TiXmlElement *root = doc.RootElement();
      
      if (root->QueryStringAttribute("machineIdentifier", &uuid) == TIXML_SUCCESS &&
          root->QueryStringAttribute("friendlyName", &name) == TIXML_SUCCESS)
      {
        CSingleLock lk(m_serverLock);
        if (m_serverMap.find(uuid) != m_serverMap.end())
          return;

        if (httpJob->m_url.GetHostName() != "127.0.0.1")
          m_manualServerUUID = uuid;

        CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, name, true));
        server->AddConnection(CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, httpJob->m_url.GetHostName(), httpJob->m_url.GetPort())));
        g_plexApplication.serverManager->UpdateFromDiscovery(server);
        m_serverMap[uuid] = server;
      }
    }
  }
  else
  {
    CSingleLock lk(m_serverLock);
    CLog::Log(LOGWARNING, "CPlexManualServerManager::OnJobComplete failed to find a server on %s", httpJob->m_url.Get().c_str());

    CPlexServerPtr server = g_plexApplication.serverManager->FindByHostAndPort(httpJob->m_url.GetHostName(), httpJob->m_url.GetPort());
    if (!server)
      return;

    if (m_serverMap.find(server->GetUUID()) != m_serverMap.end())
      m_serverMap.erase(server->GetUUID());

    server.reset();
  }

  updateServerManager();
}
