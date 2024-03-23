/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RepositoryUpdater.h"

#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonEvents.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/Repository.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
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
#include "utils/JobManager.h"
#include "utils/ProgressJob.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <vector>

using namespace std::chrono_literals;

namespace ADDON
{

class CRepositoryUpdateJob : public CProgressJob
{
public:
  explicit CRepositoryUpdateJob(const RepositoryPtr& repo) : m_repo(repo) {}
  ~CRepositoryUpdateJob() override = default;
  bool DoWork() override;
  const RepositoryPtr& GetAddon() const { return m_repo; }

private:
  const RepositoryPtr m_repo;
};

bool CRepositoryUpdateJob::DoWork()
{
  CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[{}] checking for updates.", m_repo->ID());
  CAddonDatabase database;
  database.Open();

  std::string oldChecksum;
  if (database.GetRepoChecksum(m_repo->ID(), oldChecksum) == -1)
    oldChecksum = "";

  const CAddonDatabase::RepoUpdateData updateData{database.GetRepoUpdateData(m_repo->ID())};
  if (updateData.lastCheckedVersion != m_repo->Version())
    oldChecksum = "";

  std::string newChecksum;
  std::vector<AddonInfoPtr> addons;
  int recheckAfter;
  auto status = m_repo->FetchIfChanged(oldChecksum, newChecksum, addons, recheckAfter);

  database.SetRepoUpdateData(
      m_repo->ID(), CAddonDatabase::RepoUpdateData(
                        CDateTime::GetCurrentDateTime(), m_repo->Version(),
                        CDateTime::GetCurrentDateTime() + CDateTimeSpan(0, 0, 0, recheckAfter)));

  MarkFinished();

  if (status == CRepository::STATUS_ERROR)
    return false;

  if (status == CRepository::STATUS_NOT_MODIFIED)
  {
    CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[{}] checksum not changed.", m_repo->ID());
    return true;
  }

  //Invalidate art.
  {
    CTextureDatabase textureDB;
    textureDB.Open();
    textureDB.BeginMultipleExecute();

    for (const auto& addon : addons)
    {
      AddonPtr oldAddon;
      if (CServiceBroker::GetAddonMgr().FindInstallableById(addon->ID(), oldAddon) && oldAddon &&
          addon->Version() > oldAddon->Version())
      {
        if (!oldAddon->Icon().empty() || !oldAddon->Art().empty() ||
            !oldAddon->Screenshots().empty())
          CLog::Log(LOGDEBUG, "CRepository: invalidating cached art for '{}'", addon->ID());

        if (!oldAddon->Icon().empty())
          textureDB.InvalidateCachedTexture(oldAddon->Icon());

        for (const auto& path : oldAddon->Screenshots())
          textureDB.InvalidateCachedTexture(path);

        for (const auto& art : oldAddon->Art())
          textureDB.InvalidateCachedTexture(art.second);
      }
    }
    textureDB.CommitMultipleExecute();
  }

  database.UpdateRepositoryContent(m_repo->ID(), m_repo->Version(), newChecksum, addons);
  return true;
}

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
  ScheduleUpdate(UpdateScheduleType::First);
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
    if (m_addonMgr.HasType(event.addonId, AddonType::REPOSITORY))
      ScheduleUpdate(UpdateScheduleType::First);
  }
}

void CRepositoryUpdater::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
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

        auto eventLog = CServiceBroker::GetEventLog();
        for (const auto &addon : updates)
        {
          if (eventLog)
            eventLog->Add(EventPtr(new CAddonManagementEvent(addon, 24068)));
        }
      }
    }

    if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_ON)
    {
      m_addonMgr.CheckAndInstallAddonUpdates(false);
    }

    ScheduleUpdate(UpdateScheduleType::Regular);

    m_events.Publish(RepositoryUpdated{});
  }
}

bool CRepositoryUpdater::CheckForUpdates(bool showProgress)
{
  VECADDONS addons;
  if (m_addonMgr.GetAddons(addons, AddonType::REPOSITORY) && !addons.empty())
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
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
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  auto job = std::find_if(m_jobs.begin(), m_jobs.end(),
      [&](CRepositoryUpdateJob* job){ return job->GetAddon()->ID() == repo->ID(); });

  if (job == m_jobs.end())
  {
    auto* job = new CRepositoryUpdateJob(repo);
    m_jobs.push_back(job);
    m_doneEvent.Reset();
    if (showProgress)
      SetProgressIndicator(job);
    CServiceBroker::GetJobManager()->AddJob(job, this, CJob::PRIORITY_LOW);
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
    m_timer.RestartAsync(2min);
    return;
  }

  CLog::Log(LOGDEBUG,"CRepositoryUpdater: running scheduled update");
  CheckForUpdates();
}

void CRepositoryUpdater::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting->GetId() == CSettings::SETTING_ADDONS_AUTOUPDATES)
    ScheduleUpdate(UpdateScheduleType::First);
}

CDateTime CRepositoryUpdater::LastUpdated() const
{
  VECADDONS repos;
  if (!m_addonMgr.GetAddons(repos, AddonType::REPOSITORY) || repos.empty())
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
  if (!m_addonMgr.GetAddons(repos, AddonType::REPOSITORY) || repos.empty())
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

void CRepositoryUpdater::ScheduleUpdate(UpdateScheduleType scheduleType)
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  m_timer.Stop(true);

  if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_NEVER)
    return;

  if (!m_addonMgr.HasAddons(AddonType::REPOSITORY))
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

  if (scheduleType == UpdateScheduleType::Regular)
  {
    // Enforce minimum hold-off time of 1 hour between regular updates - this is especially
    // important to handle all sorts of failure cases (e.g., failure to update the add-on database)
    // that would otherwise lead to an immediate new update attempt and continuous hammering of the servers.
    delta = std::max(1 * 60 * 60 * 1'000, delta);
  }
  else
  {
    // delta must be positive and not zero (m_timer.Start() ignores 0 wait time)
    delta = std::max(1, delta);
  }

  CLog::Log(LOGDEBUG, "CRepositoryUpdater: checking in {} ms", delta);

  if (!m_timer.Start(std::chrono::milliseconds(delta)))
    CLog::Log(LOGERROR,"CRepositoryUpdater: failed to start timer");
}
}
