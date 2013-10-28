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
#include "settings/GUISettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "ApplicationMessenger.h"
#include "filesystem/SpecialProtocol.h"
#include "PlexApplication.h"
#include "Client/MyPlex/MyPlexManager.h"

#include "xbmc/Util.h"
#include "XFileUtils.h"

using namespace XFILE;

CPlexAutoUpdate::CPlexAutoUpdate(const CURL &updateUrl, int searchFrequency)
  : m_forced(false), m_isSearching(false), m_isDownloading(false), m_url(updateUrl), m_searchFrequency(searchFrequency), m_timer(this), m_ready(false)
{
#ifdef TARGET_DARWIN_OSX
  m_timer.Start(5 * 1000, true);
#endif
}

void CPlexAutoUpdate::OnTimeout()
{
  CFileItemList list;
  CPlexDirectory dir;
  m_isSearching = true;

  m_url.SetOption("version", PLEX_VERSION);
  m_url.SetOption("build", PLEX_BUILD_TAG);
  m_url.SetOption("channel", "6");
  if (g_plexApplication.myPlexManager->IsSignedIn())
    m_url.SetOption("X-Plex-Token", g_plexApplication.myPlexManager->GetAuthToken());

  if (dir.GetDirectory(m_url, list))
  {
    m_isSearching = false;

    if (list.Size() == 1)
    {
      CFileItemPtr updateItem = list.Get(0);
      if (updateItem->HasProperty("version") &&
          updateItem->GetProperty("version").asString() != PLEX_VERSION)
      {
        CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnTimeout got version %s from update endpoint", updateItem->GetProperty("version").asString().c_str());

        DownloadUpdate(updateItem);
        return;
      }
    }
  }

  CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnTimeout no updates available");

  if (m_forced)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "No update available!", "You are up-to-date!", 10000, false);
    m_forced = false;
  }

  if (g_guiSettings.GetBool("updates.auto"))
    m_timer.SetTimeout(m_searchFrequency);
  else
    m_timer.Stop();

  m_isSearching = false;
}

CFileItemPtr CPlexAutoUpdate::GetPackage(CFileItemPtr updateItem)
{
  CFileItemPtr deltaItem, fullItem;
  if (updateItem && updateItem->m_mediaItems.size() > 0)
  {
    for (int i = 0; i < updateItem->m_mediaItems.size(); i ++)
    {
      CFileItemPtr package = updateItem->m_mediaItems[i];
      if (package->GetProperty("delta").asBoolean())
        deltaItem = package;
      else
        fullItem = package;
    }
  }

  if (deltaItem)
    return deltaItem;

  return fullItem;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexAutoUpdate::NeedDownload(const std::string& localFile, const std::string& expectedHash)
{
  if (CFile::Exists(localFile, false) && PlexUtils::GetSHA1SumFromURL(CURL(localFile)) == expectedHash)
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate we already have %s with correct SHA", localFile.c_str());
    return false;
  }
  return true;
}

void CPlexAutoUpdate::DownloadUpdate(CFileItemPtr updateItem)
{
  if (m_downloadItem)
    return;

  m_downloadPackage = GetPackage(updateItem);
  if (!m_downloadPackage)
    return;

  m_isDownloading = true;
  m_downloadItem = updateItem;
  m_needManifest = m_needBinary = m_needApplication = false;

  CDirectory::Create("special://temp/autoupdate");

  CStdString manifestUrl = m_downloadPackage->GetProperty("manifestPath").asString();
  CStdString updateUrl = m_downloadPackage->GetProperty("filePath").asString();
//  CStdString applicationUrl = m_downloadItem->GetProperty("updateApplication").asString();

  bool isDelta = m_downloadPackage->GetProperty("delta").asBoolean();
  std::string packageStr = isDelta ? "delta" : "full";
  m_localManifest = "special://temp/autoupdate/manifest-" + m_downloadItem->GetProperty("version").asString() + "." + packageStr + ".xml";
  m_localBinary = "special://temp/autoupdate/binary-" + m_downloadItem->GetProperty("version").asString() + "." + packageStr + ".zip";

  if (NeedDownload(m_localManifest, m_downloadPackage->GetProperty("manifestHash").asString()))
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate need %s", manifestUrl.c_str());
    CJobManager::GetInstance().AddJob(new CPlexDownloadFileJob(manifestUrl, m_localManifest), this, CJob::PRIORITY_LOW);
    m_needManifest = true;
  }

  if (NeedDownload(m_localBinary, m_downloadPackage->GetProperty("fileHash").asString()))
  {
    CLog::Log(LOGDEBUG, "CPlexAutoUpdate::DownloadUpdate need %s", m_localBinary.c_str());
    CJobManager::GetInstance().AddJob(new CPlexDownloadFileJob(updateUrl, m_localBinary), this, CJob::PRIORITY_LOW);
    m_needBinary = true;
  }

  if (!m_needBinary && !m_needManifest)
    ProcessDownloads();
}

void CPlexAutoUpdate::ProcessDownloads()
{
  CStdString verStr;
  verStr.Format("Version %s is now ready to be installed.", GetUpdateVersion());
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Update available!", verStr, 10000, false);

  CGUIMessage msg(GUI_MSG_UPDATE_MAIN_MENU, PLEX_AUTO_UPDATER, 0);
  CApplicationMessenger::Get().SendGUIMessage(msg, WINDOW_HOME);

  m_isDownloading = false;
  m_ready = true;
  m_timer.Stop(); // no need to poll for any more updates
}

void CPlexAutoUpdate::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexDownloadFileJob *fj = static_cast<CPlexDownloadFileJob*>(job);
  if (fj && success)
  {
    if (fj->m_destination == m_localManifest)
    {
      if (NeedDownload(m_localManifest, m_downloadPackage->GetProperty("manifestHash").asString()))
      {
        CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to download manifest, SHA mismatch. Retrying in %d seconds", m_searchFrequency);
        return;
      }

      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete got manifest.");
      m_needManifest = false;
    }
    else if (fj->m_destination == m_localBinary)
    {
      if (NeedDownload(m_localBinary, m_downloadPackage->GetProperty("fileHash").asString()))
      {
        CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to download update, SHA mismatch. Retrying in %d seconds", m_searchFrequency);
        return;
      }

      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete got update binary.");
      m_needBinary = false;
    }
    else
      CLog::Log(LOGDEBUG, "CPlexAutoUpdate::OnJobComplete What is %s", fj->m_destination.c_str());
  }
  else if (!success)
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdate::OnJobComplete failed to run a download job, will retry in %d seconds.", m_searchFrequency);
    return;
  }

  if (!m_needApplication && !m_needBinary && !m_needManifest)
    ProcessDownloads();
}

void CPlexAutoUpdate::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
}

bool CPlexAutoUpdate::RenameLocalBinary()
{
  CXBMCTinyXML doc;

  doc.LoadFile(m_localManifest);
  if (!doc.RootElement())
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdate::RenameLocalBinary failed to parse mainfest!");
    return false;
  }

  std::string newName;
  TiXmlElement *el = doc.RootElement()->FirstChildElement();
  while(el)
  {
    if (el->ValueStr() == "packages" || el->ValueStr() == "package")
    {
      el=el->FirstChildElement();
      continue;
    }
    if (el->ValueStr() == "name")
    {
      newName = el->GetText();
      break;
    }

    el = el->NextSiblingElement();
  }

  if (newName.empty())
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdater::RenameLocalBinary failed to get the new name from the manifest!");
    return false;
  }

  std::string bpath = CSpecialProtocol::TranslatePath(m_localBinary);
  std::string tgd = CSpecialProtocol::TranslatePath("special://temp/autoupdate/" + newName + ".zip");

  return CopyFile(bpath.c_str(), tgd.c_str(), false);
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
  /* first we need to copy the updater app to our tmp directory, it might change during install.. */

  CStdString updaterPath;
  CUtil::GetHomePath(updaterPath);
  updaterPath += "/tools/updater";
  std::string updater = CSpecialProtocol::TranslatePath("special://temp/autoupdate/updater");

  if (!CopyFile(updaterPath.c_str(), updater.c_str(), false))
  {
    CLog::Log(LOGWARNING, "CPlexAutoUpdate::UpdateAndRestart failed to copy %s to %s", updaterPath.c_str(), updater.c_str());
    return;
  }

#ifdef TARGET_POSIX
  chmod(updater.c_str(), 0755);
#endif

  if (!RenameLocalBinary())
    return;

  std::string script = CSpecialProtocol::TranslatePath(m_localManifest);
  std::string packagedir = CSpecialProtocol::TranslatePath("special://temp/autoupdate");
  std::string appdir;

#ifdef TARGET_DARWIN_OSX
  char installdir[2*MAXPATHLEN];

  uint32_t size;
  GetDarwinBundlePath(installdir, &size);
  appdir = std::string(installdir) + "/..";
#endif

  CStdString exec;
  exec.Format("\"%s\" --install-dir \"%s\" --package-dir \"%s\" --script \"%s\" --auto-close", updater, appdir, packagedir, script);
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

void CPlexAutoUpdate::ForceVersionCheckInBackground()
{
  if (m_timer.IsRunning())
    m_timer.Stop(true);

  m_forced = true;
  m_isSearching = true;
  // restart with a short time out, just to make sure that we get it running in the background thread
  m_timer.Start(1);
}

void CPlexAutoUpdate::ResetTimer()
{
  if (g_guiSettings.GetBool("updates.auto"))
  {
    if (m_timer.IsRunning())
      m_timer.Stop(true);
    m_timer.Start(m_searchFrequency);
  }
}
