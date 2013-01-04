//
//  PlexAutoUpdate.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdate.h"
#include <boost/foreach.hpp>

#ifdef __APPLE__
#include "Mac/PlexAutoUpdateMac.h"
#endif

#ifdef TARGET_WINDOWS
#include "Win\PlexAutoUpdateInstallerWin.h"
#endif

CPlexAutoUpdate::CPlexAutoUpdate(const std::string &updateUrl, int searchFrequency) :
  m_updateUrl(updateUrl),
  m_searchFrequency(searchFrequency),
  m_stop(false),
  m_currentVersion(PLEX_VERSION),
  m_autoUpdateThread(boost::bind(&CPlexAutoUpdate::run, this))
{
  m_functions = new CAutoUpdateFunctionsXBMC(this);
#ifdef __APPLE__
  m_installer = new CPlexAutoUpdateInstallerMac(m_functions);
#elif defined(TARGET_WINDOWS)
  m_installer = new CPlexAutoUpdateInstallerWin(m_functions);
#endif

  m_autoUpdateThread.detach();

}

void CPlexAutoUpdate::Stop()
{
  {
    boost::mutex::scoped_lock lk(m_lock);
    m_stop = true;
    m_waitSleepCond.notify_one();
    m_functions->LogDebug("Killing autoupdate thread...");
  }
  m_autoUpdateThread.join();
}

void CPlexAutoUpdate::run()
{
  m_functions->LogDebug("Thread is running...");

  {
    // Waiting 10 seconds before polling for updates
    boost::mutex::scoped_lock lk(m_lock);
    m_waitSleepCond.timed_wait(lk, boost::posix_time::seconds(10));
  }

  while (!m_stop)
  {
    boost::mutex::scoped_lock lk(m_lock);

    _CheckForNewVersion();

    boost::system_time const tmout = boost::get_system_time() + boost::posix_time::seconds(m_searchFrequency);
    m_functions->LogDebug("Thread is going back to sleep");
    m_waitSleepCond.timed_wait(lk, tmout);
  }
}

bool CPlexAutoUpdate::DownloadNewVersion()
{
  std::string localPath;
  std::string destination;
  if (m_functions->DownloadFile(m_newVersion.m_enclosureUrl, localPath))
  {
    // Successful download
    if(m_functions->ShouldWeInstall(localPath))
    {
      m_installer->InstallUpdate(localPath, destination);
    }
  }
  
  return true;
}

std::string CPlexAutoUpdate::GetOsName() const
{
#ifdef TARGET_DARWIN_OSX
  return "osx";
#elif defined(TARGET_WINDOWS)
  return "windows";
#elif defined(TARGET_LINUX)
  return "linux";
#endif
}

bool CPlexAutoUpdate::_CheckForNewVersion()
{
  std::string data;
  m_functions->LogInfo("Checking for new version");
  if (m_functions->FetchUrlData(m_updateUrl, data))
  {
    CAutoUpdateInfoList list;
    if (m_functions->ParseXMLData(data, list))
    {
      BOOST_FOREACH(CAutoUpdateInfo info, list)
      {
        if (info.m_enclosureOs != GetOsName())
          continue;

        m_functions->LogInfo("Found version " + info.m_enclosureVersion.GetVersionString());

        if(m_currentVersion < info.m_enclosureVersion)
        {
          m_functions->LogInfo("Found new version " + info.m_enclosureVersion.GetVersionString());
          m_newVersion = info;
          m_functions->NotifyNewVersion();

          return true;
        }
      }
    }
  }
  m_functions->LogInfo("No new version found!");
  return false;
}
