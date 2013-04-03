#pragma once

#include "Client/PlexServer.h"
#include "Client/PlexServerManager.h"

#include "GlobalsHandling.h"
#include "JobManager.h"

#include <boost/shared_ptr.hpp>
#include <map>

#include "FileItem.h"
#include "plex/PlexTypes.h"

/* Maps UUID->list of sections for the section */
typedef std::map<CStdString, CFileItemListPtr> ServerDataMap;
typedef std::pair<CStdString, CFileItemListPtr> ServerDataPair;

class CPlexServerDataLoaderJob : public CJob
{
public:
  CPlexServerDataLoaderJob(const CPlexServerPtr& server) : m_server(server) {}

  bool DoWork();
  CFileItemListPtr FetchList(const CStdString& path);

  CPlexServerPtr m_server;
  CFileItemListPtr m_sectionList;
  CFileItemListPtr m_channelList;

  virtual bool operator==(const CJob* job) const
  {
    CPlexServerDataLoaderJob *oJob = (CPlexServerDataLoaderJob*)job;
    if (oJob->m_server == m_server)
      return true;
    return false;
  }
};

class CPlexServerDataLoader : public CJobQueue
{
public:
  CPlexServerDataLoader();

  void LoadDataFromServer(const CPlexServerPtr& server);

  CFileItemListPtr GetSectionsForUUID(const CStdString& uuid);
  CFileItemListPtr GetSectionsForServer(const CPlexServerPtr &server) { return GetSectionsForUUID(server->GetUUID()); }
  CFileItemListPtr GetChannelsForUUID(const CStdString& uuid);
  CFileItemListPtr GetChannelsForServer(const CPlexServerPtr &server) { return GetChannelsForUUID(server->GetUUID()); }

  CFileItemListPtr GetAllSections() const;

  void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CCriticalSection m_dataLock;

  ServerDataMap m_sectionMap;
  ServerDataMap m_channelMap;
};

XBMC_GLOBAL_REF(CPlexServerDataLoader, g_plexServerDataLoader);
#define g_plexServerDataLoader XBMC_GLOBAL_USE(CPlexServerDataLoader)
