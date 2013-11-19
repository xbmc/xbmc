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
#include "PlexApplication.h"

class CPlexAutoUpdate : public IJobCallback, public IPlexGlobalTimeout
{
  public:
    CPlexAutoUpdate();
    void OnTimeout();
    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

    bool IsReadyToInstall() const { return m_ready; }
    bool IsDownloadingUpdate() const { return m_isDownloading; }
    CStdString GetUpdateVersion() const { return m_downloadItem->GetProperty("version").asString(); }
    bool IsSearchingForUpdate() const { return m_isSearching; }
    void UpdateAndRestart();
    void ForceVersionCheckInBackground();
    void ResetTimer();

    void PokeFromSettings();

    void WriteUpdateInfo();
    bool GetUpdateInfo(std::string& version, bool& isDelta, std::string& packageHash, std::string& fromVersion) const;

    int GetDownloadPercentage() const { return m_percentage; }

  private:
    void CheckInstalledVersion();
    void DownloadUpdate(CFileItemPtr updateItem);
    void ProcessDownloads();

    CFileItemPtr m_downloadItem;
    CFileItemPtr m_downloadPackage;
    uint32_t m_searchFrequency;
    CURL m_url;

    CStdString m_localManifest;
    CStdString m_localBinary;
    CStdString m_localApplication;

    bool m_forced;
    bool m_isSearching;
    bool m_isDownloading;

    bool m_needManifest;
    bool m_needBinary;
    bool m_needApplication;

    bool m_ready;

    CFileItemPtr GetPackage(CFileItemPtr updateItem);
    bool NeedDownload(const std::string& localFile, const std::string& expectedHash);
    bool RenameLocalBinary();
    int m_percentage;

    std::vector<std::string> GetAllInstalledVersions() const;
};

#endif // PLEXAUTOUPDATE_H
