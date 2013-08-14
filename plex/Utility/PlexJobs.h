//
//  PlexJobs.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#ifndef __Plex_Home_Theater__PlexJobs__
#define __Plex_Home_Theater__PlexJobs__

#include "utils/Job.h"
#include "URL.h"
#include "filesystem/CurlFile.h"

class CPlexHTTPFetchJob : public CJob
{
  public:
    CPlexHTTPFetchJob(const CURL &url) : CJob(), m_url(url) {};
  
    bool DoWork();
    virtual bool operator==(const CJob* job) const;
  
    XFILE::CCurlFile m_http;
    CStdString m_data;
    CURL m_url;
};


#endif /* defined(__Plex_Home_Theater__PlexJobs__) */
