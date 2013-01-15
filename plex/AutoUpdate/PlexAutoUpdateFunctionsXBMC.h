//
//  PlexAutoUpdateFunctionsXBMC.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-09.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATEFUNCTIONSXBMC_H
#define PLEXAUTOUPDATEFUNCTIONSXBMC_H

#include "plex/AutoUpdate/PlexAutoUpdate.h"
#include "utils/XBMCTinyXML.h"
#include "Job.h"
#include "JobManager.h"
#include "dialogs/GUIDialogProgress.h"


class CAutoUpdateFunctionsXBMC : public CAutoUpdateFunctionsBase, IJobCallback
{
   public:

    CAutoUpdateFunctionsXBMC(CPlexAutoUpdate* updater) : CAutoUpdateFunctionsBase(updater) {};
    virtual bool FetchUrlData(const std::string &url, std::string& data);
    virtual bool ParseXMLData(const std::string &xmlData, CAutoUpdateInfoList &infoList);
    virtual void LogDebug(const std::string& msg);
    virtual void LogInfo(const std::string &msg);
    virtual bool DownloadFile(const std::string &url, std::string &localPath);
  
    virtual std::string GetResourcePath() const;

    virtual void NotifyNewVersion();

    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    virtual void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);
  
    virtual bool ShouldWeInstall(const std::string& localPath);
    virtual void TerminateApplication();

  private:
    std::string GetLocalFileName(const std::string& baseName);
    bool ParseItemElement(TiXmlElement *el, CAutoUpdateInfo& info);
    CGUIDialogProgress* m_progressDialog;
    bool m_downloadingDone;
    bool m_downloadSuccess;
    int m_jobId;
};

#endif // PLEXAUTOUPDATEFUNCTIONSXBMC_H
