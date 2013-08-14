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

class CPlexManualServerManager : IJobCallback
{
  public:
    CPlexManualServerManager() {}
    void checkManualServersAsync();
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);
};

#endif /* defined(__Plex_Home_Theater__PlexManualServerManager__) */
