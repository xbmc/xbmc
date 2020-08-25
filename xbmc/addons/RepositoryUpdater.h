/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "addons/AddonEvents.h"
#include "addons/Repository.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"
#include "utils/EventStream.h"

#include <vector>

namespace ADDON
{

class CRepositoryUpdater : private ITimerCallback, private IJobCallback, public ISettingCallback
{
public:
  explicit CRepositoryUpdater(CAddonMgr& addonMgr);
  ~CRepositoryUpdater() override;

  void Start();

  /**
   * Check a single repository for updates.
   */
  void CheckForUpdates(const ADDON::RepositoryPtr& repo, bool showProgress=false);

  /**
   * Check all repositories for updates.
   */
  bool CheckForUpdates(bool showProgress=false);

  /**
   * Wait for any pending/in-progress updates to complete.
   */
  void Await();

  /**
   * Schedule an automatic update to run based on settings and previous update
   * times. May be called when there are external changes this updater must know
   * about. Any previously scheduled update will be cancelled.
   */
  void ScheduleUpdate();

  /**
   * Returns the time of the last check (oldest). Invalid time if never checked.
   */
  CDateTime LastUpdated() const;


  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  struct RepositoryUpdated { };

  CEventStream<RepositoryUpdated>& Events() { return m_events; }

private:
  CRepositoryUpdater(const CRepositoryUpdater&) = delete;
  CRepositoryUpdater(CRepositoryUpdater&&) = delete;
  CRepositoryUpdater& operator=(const CRepositoryUpdater&) = delete;
  CRepositoryUpdater& operator=(CRepositoryUpdater&&) = delete;

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  void OnTimeout() override;

  void OnEvent(const ADDON::AddonEvent& event);

  CDateTime ClosestNextCheck() const;

  CCriticalSection m_criticalSection;
  CTimer m_timer;
  CEvent m_doneEvent;
  std::vector<CRepositoryUpdateJob*> m_jobs;
  CAddonMgr& m_addonMgr;

  CEventSource<RepositoryUpdated> m_events;
};
}
