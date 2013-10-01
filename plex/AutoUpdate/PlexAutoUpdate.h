//
//  PlexAutoUpdate.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATE_H
#define PLEXAUTOUPDATE_H

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "threads/Timer.h"
#include "Job.h"
#include "URL.h"
#include "FileItem.h"

#include "threads/Thread.h"

class CPlexAutoUpdate : public ITimerCallback, IJobCallback
{
  public:
    CPlexAutoUpdate(const CURL& updateUrl, int searchFrequency = 21600);
    void OnTimeout();
    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

    bool IsReadyToInstall() const { return m_ready; }
    void UpdateAndRestart();

  private:
    void DownloadUpdate(CFileItemPtr updateItem);
    void ProcessDownloads();

    CFileItemPtr m_downloadItem;
    int m_searchFrequency;
    CURL m_url;
    CTimer m_timer;

    CStdString m_localManifest;
    CStdString m_localBinary;
    CStdString m_localApplication;

    bool m_needManifest;
    bool m_needBinary;
    bool m_needApplication;

    bool m_ready;
};

#endif // PLEXAUTOUPDATE_H
