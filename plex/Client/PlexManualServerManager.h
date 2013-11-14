//
//  PlexManualServerManager.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#ifndef __Plex_Home_Theater__PlexManualServerManager__
#define __Plex_Home_Theater__PlexManualServerManager__

#include <string>
#include "utils/JobManager.h"
#include <map>
#include "threads/CriticalSection.h"
#include "PlexTypes.h"

class CPlexManualServerManager : IJobCallback
{
  public:
    CPlexManualServerManager() : m_manualServerUUID("") {}
    void checkManualServersAsync();
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    void updateServerManager();
    void checkLocalhost();

    CCriticalSection m_serverLock;
    PlexServerMap m_serverMap;
    std::string m_manualServerUUID;
};

#endif /* defined(__Plex_Home_Theater__PlexManualServerManager__) */
