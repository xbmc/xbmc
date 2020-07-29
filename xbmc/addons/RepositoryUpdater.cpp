/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RepositoryUpdater.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace ADDON
{

CRepositoryUpdater::CRepositoryUpdater(CAddonMgr& addonMgr) :
  m_timer(this),
  m_doneEvent(true),
  m_addonMgr(addonMgr)
{
  // Register settings
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_ADDONS_AUTOUPDATES);
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, settingSet);
}

void CRepositoryUpdater::Start()
{
  m_addonMgr.Events().Subscribe(this, &CRepositoryUpdater::OnEvent);
  ScheduleUpdate();
}

CRepositoryUpdater::~CRepositoryUpdater()
{
  // Unregister settings
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);

  m_addonMgr.Events().Unsubscribe(this);
}

void CRepositoryUpdater::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled))
  {
    if (m_addonMgr.HasType(event.id, ADDON_REPOSITORY))
      ScheduleUpdate();
  }
}

void CRepositoryUpdater::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CSingleLock lock(m_criticalSection);
  m_jobs.erase(std::find(m_jobs.begin(), m_jobs.end(), job));
  if (m_jobs.empty())
  {
    CLog::Log(LOGDEBUG, "CRepositoryUpdater: done.");
    m_doneEvent.Set();

    VECADDONS updates = m_addonMgr.GetAvailableUpdates();

    if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_NOTIFY)
    {
      if (!updates.empty())
      {
        if (updates.size() == 1)
          CGUIDialogKaiToast::QueueNotification(
              updates[0]->Icon(), updates[0]->Name(), g_localizeStrings.Get(24068),
              TOAST_DISPLAY_TIME, false, TOAST_DISPLAY_TIME);
        else
          CGUIDialogKaiToast::QueueNotification(
              "", g_localizeStrings.Get(24001), g_localizeStrings.Get(24061),
              TOAST_DISPLAY_TIME, false, TOAST_DISPLAY_TIME);

        for (const auto &addon : updates)
          CServiceBroker::GetEventLog().Add(EventPtr(new CAddonManagementEvent(addon, 24068)));
      }
    }

    if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_ON)
    {
      m_addonMgr.CheckAndInstallAddonUpdates(false);
    }

    ScheduleUpdate();

    m_events.Publish(RepositoryUpdated{});
  }
}

bool CRepositoryUpdater::CheckForUpdates(bool showProgress)
{
  VECADDONS addons;
  if (m_addonMgr.GetAddons(addons, ADDON_REPOSITORY) && !addons.empty())
  {
    CSingleLock lock(m_criticalSection);
    for (const auto& addon : addons)
      CheckForUpdates(std::static_pointer_cast<ADDON::CRepository>(addon), showProgress);

    return true;
  }

  return false;
}

static void SetProgressIndicator(CRepositoryUpdateJob* job)
{
  auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
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
  if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME ||
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW)
  {
    CLog::Log(LOGDEBUG,"CRepositoryUpdater: busy playing. postponing scheduled update");
    m_timer.RestartAsync(2 * 60 * 1000);
    return;
  }

  CLog::Log(LOGDEBUG,"CRepositoryUpdater: running scheduled update");
  CheckForUpdates();
}

void CRepositoryUpdater::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting->GetId() == CSettings::SETTING_ADDONS_AUTOUPDATES)
    ScheduleUpdate();
}

CDateTime CRepositoryUpdater::LastUpdated() const
{
  VECADDONS repos;
  if (!m_addonMgr.GetAddons(repos, ADDON_REPOSITORY) || repos.empty())
    return CDateTime();

  CAddonDatabase db;
  db.Open();
  std::vector<CDateTime> updateTimes;
  std::transform(
      repos.begin(), repos.end(), std::back_inserter(updateTimes), [&](const AddonPtr& repo) {
        const auto updateData = db.GetRepoUpdateData(repo->ID());
        if (updateData.lastCheckedAt.IsValid() && updateData.lastCheckedVersion == repo->Version())
          return updateData.lastCheckedAt;
        return CDateTime();
      });

  return *std::min_element(updateTimes.begin(), updateTimes.end());
}

CDateTime CRepositoryUpdater::ClosestNextCheck() const
{
  VECADDONS repos;
  if (!m_addonMgr.GetAddons(repos, ADDON_REPOSITORY) || repos.empty())
    return CDateTime();

  CAddonDatabase db;
  db.Open();
  std::vector<CDateTime> nextCheckTimes;
  std::transform(
      repos.begin(), repos.end(), std::back_inserter(nextCheckTimes), [&](const AddonPtr& repo) {
        const auto updateData = db.GetRepoUpdateData(repo->ID());
        if (updateData.nextCheckAt.IsValid() && updateData.lastCheckedVersion == repo->Version())
          return updateData.nextCheckAt;
        return CDateTime();
      });

  return *std::min_element(nextCheckTimes.begin(), nextCheckTimes.end());
}

void CRepositoryUpdater::ScheduleUpdate()
{
  CSingleLock lock(m_criticalSection);
  m_timer.Stop(true);

  if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_NEVER)
    return;

  if (!m_addonMgr.HasAddons(ADDON_REPOSITORY))
    return;

  int delta{1};
  const auto nextCheck = ClosestNextCheck();
  if (nextCheck.IsValid())
  {
    // Repos were already checked once and we know when to check next
    delta = std::max(1, (nextCheck - CDateTime::GetCurrentDateTime()).GetSecondsTotal() * 1000);
    CLog::Log(LOGDEBUG, "CRepositoryUpdater: closest next update check at {} (in {} s)",
              nextCheck.GetAsLocalizedDateTime(), delta / 1000);
  }

  if (!m_timer.Start(delta))
    CLog::Log(LOGERROR,"CRepositoryUpdater: failed to start timer");
}
}
