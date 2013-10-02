//
//  PlexAutoUpdate.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdate.h"
#include <boost/foreach.hpp>
#include "FileSystem/PlexDirectory.h"
#include "FileItem.h"
#include "PlexJobs.h"
#include "File.h"
#include "Directory.h"
#include "utils/URIUtils.h"

#include "dialogs/GUIDialogKaiToast.h"

#include "ApplicationMessenger.h"

#include "filesystem/SpecialProtocol.h"

using namespace XFILE;

CPlexAutoUpdate::CPlexAutoUpdate(const CURL &updateUrl, int searchFrequency) : m_url(updateUrl), m_searchFrequency(searchFrequency), m_timer(this), m_ready(false)
{
#ifdef TARGET_DARWIN_OSX
  m_timer.Start(5 * 1000, false);
#endif
}

void CPlexAutoUpdate::OnTimeout()
{
  CFileItemList list;
  CPlexDirectory dir;
  if (dir.GetDirectory(m_url, list))
  {
    if (list.Size() != 1)
    {
      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnTimeout failed to get something useful from %s", m_url.Get().c_str());
      return;
    }

    CFileItemPtr updateItem = list.Get(0);
    if (updateItem->HasProperty("version") &&
        updateItem->GetProperty("version").asString() != PLEX_VERSION)
    {
      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnTimeout got version %s from update endpoint", updateItem->GetProperty("version").asString().c_str());

      DownloadUpdate(updateItem);
    }
    else
      m_timer.Start(m_searchFrequency);
  }
}

void CPlexAutoUpdate::DownloadUpdate(CFileItemPtr updateItem)
{
  if (m_downloadItem)
    return;

  m_downloadItem = updateItem;
  m_needManifest = m_needBinary = m_needApplication = false;

  CDirectory::Create("special://temp/autoupdate");

  CStdString manifestUrl = m_downloadItem->GetProperty("updateManifest").asString();
  CStdString updateUrl = m_downloadItem->GetProperty("updateBinary").asString();
  CStdString applicationUrl = m_downloadItem->GetProperty("updateApplication").asString();

  m_localManifest = "special://temp/autoupdate/" + URIUtils::GetFileName(manifestUrl);
  m_localBinary = "special://temp/autoupdate/" + URIUtils::GetFileName(updateUrl);
  m_localApplication = "special://temp/autoupdate/updater";

  if (CFile::Exists(m_localManifest) && PlexUtils::GetSHA1SumFromURL(m_localManifest) == m_downloadItem->GetProperty("updateManifestSHA").asString())
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate we already have %s with correct SHA", m_localManifest.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate need %s", manifestUrl.c_str());
    CJobManager::GetInstance().AddJob(new CPlexDownloadFileJob(manifestUrl, m_localManifest), this, CJob::PRIORITY_LOW);
    m_needManifest = true;
  }

  if (CFile::Exists(m_localBinary) && PlexUtils::GetSHA1SumFromURL(m_localBinary) == m_downloadItem->GetProperty("updateBinarySHA").asString())
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate we already have %s with correct SHA", m_localBinary.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate need %s", updateUrl.c_str());
    CJobManager::GetInstance().AddJob(new CPlexDownloadFileJob(updateUrl, m_localBinary), this, CJob::PRIORITY_LOW);
    m_needBinary = true;
  }

  if (CFile::Exists(m_localApplication) && PlexUtils::GetSHA1SumFromURL(m_localApplication) == m_downloadItem->GetProperty("updateApplicationSHA").asString())
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate we already have %s with correct SHA", m_localApplication.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate need %s", applicationUrl.c_str());
    CJobManager::GetInstance().AddJob(new CPlexDownloadFileJob(applicationUrl, m_localApplication), this, CJob::PRIORITY_LOW);
    m_needApplication = true;
  }

  if (!m_needBinary && !m_needManifest && !m_needApplication)
    ProcessDownloads();
}

void CPlexAutoUpdate::ProcessDownloads()
{
  CStdString verStr;
  verStr.Format("Version %s is now ready to be installed.", m_downloadItem->GetProperty("version").asString());
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Update available!", verStr, 10000, false);

  CGUIMessage msg(GUI_MSG_UPDATE_MAIN_MENU, PLEX_AUTO_UPDATER, 0);
  CApplicationMessenger::Get().SendGUIMessage(msg, WINDOW_HOME);

  m_ready = true;
}

void CPlexAutoUpdate::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexDownloadFileJob *fj = static_cast<CPlexDownloadFileJob*>(job);
  if (fj && success)
  {
    if (fj->m_destination == m_localManifest)
    {
      if (PlexUtils::GetSHA1SumFromURL(m_localManifest) != m_downloadItem->GetProperty("updateManifestSHA").asString())
      {
        CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to download manifest, SHA mismatch. Retrying in %d seconds", m_searchFrequency);
        m_timer.Start(m_searchFrequency);
      }

      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete got manifest.");
      m_needManifest = false;
    }
    else if (fj->m_destination == m_localBinary)
    {
      if (PlexUtils::GetSHA1SumFromURL(m_localBinary) != m_downloadItem->GetProperty("updateBinarySHA").asString())
      {
        CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to download update, SHA mismatch. Retrying in %d seconds", m_searchFrequency);
        m_timer.Start(m_searchFrequency);
      }

      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete got update binary.");
      m_needBinary = false;
    }
    else if (fj->m_destination == m_localApplication)
    {
      if (PlexUtils::GetSHA1SumFromURL(m_localApplication) != m_downloadItem->GetProperty("updateApplicationSHA").asString())
      {
        CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to download updater, SHA mismatch. Retrying in %d seconds", m_searchFrequency);
        m_timer.Start(m_searchFrequency);
      }
      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete got update application");
      m_needApplication = false;

#if defined(TARGET_POSIX)
      chmod(CSpecialProtocol::TranslatePath(m_localApplication), 0755);
#endif

    }
    else
      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete What is %s", fj->m_destination.c_str());
  }
  else if (!success)
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to run a download job, will retry in %d seconds.", m_searchFrequency);
    m_timer.Start(m_searchFrequency);
  }

  if (!m_needApplication && !m_needBinary && !m_needManifest)
    ProcessDownloads();
}

void CPlexAutoUpdate::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
}

#ifdef TARGET_POSIX
#include <signal.h>
#endif

#ifdef TARGET_DARWIN_OSX
#include "DarwinUtils.h"
#endif
 
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

void CPlexAutoUpdate::UpdateAndRestart()
{
  std::string updater = CSpecialProtocol::TranslatePath(m_localApplication);
  std::string script = CSpecialProtocol::TranslatePath(m_localManifest);
  std::string packagedir = CSpecialProtocol::TranslatePath("special://temp/autoupdate");
  char installdir[2*MAXPATHLEN];

#ifdef TARGET_DARWIN_OSX
  uint32_t size;
  GetDarwinBundlePath(installdir, &size);
#endif

  CStdString exec;
  exec.Format("\"%s\" --install-dir \"%s\" --package-dir \"%s\" --script \"%s\"", updater, installdir, packagedir, script);
  CLog::Log(LOGDEBUG, "CPlexAutoUpdate::UpdateAndRestart going to run %s", exec.c_str());

#ifdef TARGET_POSIX
  pid_t pid = fork();
  if (pid == -1)
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdate::UpdateAndRestart major fail when installing update, can't fork!");
    return;
  }
  else if (pid == 0)
  {
    /* Child */
    pid_t parentPid = getppid();
    fprintf(stderr, "Waiting for PHT to quit...\n");
    while(kill(parentPid, SIGINT) == 0)
      sleep(1);

    fprintf(stderr, "PHT seems to have quit, running updater\n");
    system(exec.c_str());

    exit(0);
  }
  else
  {
    CApplicationMessenger::Get().Quit();
  }

#endif
}
