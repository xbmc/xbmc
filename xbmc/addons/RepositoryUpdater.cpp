/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RepositoryUpdater.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include <algorithm>
#include <vector>

namespace ADDON
{

CRepositoryUpdater::CRepositoryUpdater() :
  m_timer(this),
  m_doneEvent(true)
{}

CRepositoryUpdater &CRepositoryUpdater::GetInstance()
{
  static CRepositoryUpdater instance;
  return instance;
}

void CRepositoryUpdater::Start()
{
  ScheduleUpdate();
}

void CRepositoryUpdater::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CSingleLock lock(m_criticalSection);
  m_jobs.erase(std::find(m_jobs.begin(), m_jobs.end(), job));
  if (m_jobs.empty())
  {
    CLog::Log(LOGDEBUG, "CRepositoryUpdater: done.");
    m_doneEvent.Set();

    if (CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_NOTIFY)
    {
      VECADDONS hasUpdate = CAddonMgr::GetInstance().GetOutdated();
      if (!hasUpdate.empty())
      {
        if (hasUpdate.size() == 1)
          CGUIDialogKaiToast::QueueNotification(
              hasUpdate[0]->Icon(), hasUpdate[0]->Name(), g_localizeStrings.Get(24068),
              TOAST_DISPLAY_TIME, false, TOAST_DISPLAY_TIME);
        else
          CGUIDialogKaiToast::QueueNotification(
              "", g_localizeStrings.Get(24001), g_localizeStrings.Get(24061),
              TOAST_DISPLAY_TIME, false, TOAST_DISPLAY_TIME);
      }
    }

    if (CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_ON)
      CAddonInstaller::GetInstance().InstallUpdates();

    ScheduleUpdate();

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendThreadMessage(msg);
  }
}

void CRepositoryUpdater::CheckForUpdates(bool showProgress)
{
  VECADDONS addons;
  if (CAddonMgr::GetInstance().GetAddons(ADDON_REPOSITORY, addons) && !addons.empty())
  {
    CSingleLock lock(m_criticalSection);
    for (const auto& addon : addons)
      CheckForUpdates(std::static_pointer_cast<ADDON::CRepository>(addon), showProgress);
  }
}

static void SetProgressIndicator(CRepositoryUpdateJob* job)
{
  auto* dialog = static_cast<CGUIDialogExtendedProgressBar*>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
  if (dialog)
    job->SetProgressIndicators(dialog->GetHandle(g_localizeStrings.Get(24092)), nullptr);
}

void CRepositoryUpdater::CheckForUpdates(const ADDON::RepositoryPtr& repo, bool showProgress)
{
  CSingleLock lock(m_criticalSection);
  auto job = std::find_if(m_jobs.begin(), m_jobs.end(),
      [&](CRepositoryUpdateJob* job){ return job->GetAddon()->ID() == repo->ID(); });

  if (job == m_jobs.end())
  {
    auto* job = new CRepositoryUpdateJob(repo);
    m_jobs.push_back(job);
    m_doneEvent.Reset();
    if (showProgress)
      SetProgressIndicator(job);
    CJobManager::GetInstance().AddJob(job, this, CJob::PRIORITY_LOW);
  }
  else
  {
    if (showProgress && !(*job)->HasProgressIndicator())
      SetProgressIndicator(*job);
  }
}

void CRepositoryUpdater::Await()
{
  m_doneEvent.Wait();
}

void CRepositoryUpdater::OnTimeout()
{
  //workaround
  if (g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      g_windowManager.GetActiveWindow() == WINDOW_SLIDESHOW)
  {
    CLog::Log(LOGDEBUG,"CRepositoryUpdater: busy playing. postponing scheduled update");
    m_timer.RestartAsync(2 * 60 * 1000);
    return;
  }

  CLog::Log(LOGDEBUG,"CRepositoryUpdater: running scheduled update");
  CheckForUpdates();
}

void CRepositoryUpdater::OnSettingChanged(const CSetting* setting)
{
  if (setting->GetId() == CSettings::SETTING_GENERAL_ADDONUPDATES)
    ScheduleUpdate();
}

CDateTime CRepositoryUpdater::LastUpdated() const
{
  VECADDONS repos;
  if (!CAddonMgr::GetInstance().GetAddons(ADDON_REPOSITORY, repos) || repos.empty())
    return CDateTime();

  CAddonDatabase db;
  db.Open();
  std::vector<CDateTime> updateTimes;
  std::transform(repos.begin(), repos.end(), std::back_inserter(updateTimes),
    [&](const AddonPtr& repo)
    {
      auto lastCheck = db.LastChecked(repo->ID());
      if (lastCheck.first.IsValid() && lastCheck.second == repo->Version())
        return lastCheck.first;
      return CDateTime();
    });

  return *std::min_element(updateTimes.begin(), updateTimes.end());
}

void CRepositoryUpdater::ScheduleUpdate()
{
  const CDateTimeSpan interval(0, 24, 0, 0);

  CSingleLock lock(m_criticalSection);
  m_timer.Stop(true);

  if (CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_NEVER)
    return;

  if (!CAddonMgr::GetInstance().HasAddons(ADDON_REPOSITORY))
    return;

  auto prev = LastUpdated();
  auto next = std::max(CDateTime::GetCurrentDateTime(), prev + interval);
  int delta = std::max(1, (next - CDateTime::GetCurrentDateTime()).GetSecondsTotal() * 1000);

  CLog::Log(LOGDEBUG,"CRepositoryUpdater: previous update at %s, next at %s",
      prev.GetAsLocalizedDateTime().c_str(), next.GetAsLocalizedDateTime().c_str());

  if (!m_timer.Start(delta))
    CLog::Log(LOGERROR,"CRepositoryUpdater: failed to start timer");
}
}
