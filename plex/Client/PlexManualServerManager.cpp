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
    /* remove all manually added servers */
    g_plexApplication.serverManager->UpdateFromConnectionType(PlexServerList(), CPlexConnection::CONNECTION_MANUAL);
}

void CPlexManualServerManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexHTTPFetchJob *httpJob = static_cast<CPlexHTTPFetchJob*>(job);
  PlexServerList list;
  
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
        CPlexServerPtr server = CPlexServerPtr(new CPlexServer(uuid, name, true));
        server->AddConnection(CPlexConnectionPtr(new CPlexConnection(CPlexConnection::CONNECTION_MANUAL, httpJob->m_url.GetHostName(), httpJob->m_url.GetPort())));
        g_plexApplication.serverManager->UpdateFromDiscovery(server);
        list.push_back(server);
      }
    }
  }
  else
    CLog::Log(LOGWARNING, "CPlexManualServerManager::OnJobComplete failed to find a server on %s", httpJob->m_url.Get().c_str());
  
  g_plexApplication.serverManager->UpdateFromConnectionType(list, CPlexConnection::CONNECTION_MANUAL);
}
