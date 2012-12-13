//
//  PlexDownloadFileJob.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-12.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXDOWNLOADFILEJOB_H
#define PLEXDOWNLOADFILEJOB_H

#include "Job.h"
#include "filesystem/CurlFile.h"

class CPlexDownloadFileJob : public CJob
{
  public:
    CPlexDownloadFileJob(const CStdString& url, const CStdString& destination) :
      CJob(), m_failed(false), m_url(url), m_destination(destination)
    {};

    bool DoWork();

  private:
    CStdString m_url;
    CStdString m_destination;

    bool m_failed;
};

#endif // PLEXDOWNLOADFILEJOB_H
