#pragma once
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

#include "addons/Repository.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"
#include "utils/EventStream.h"
#include "XBDateTime.h"
#include <vector>

namespace ADDON
{

class CRepositoryUpdater : private ITimerCallback, private IJobCallback, public ISettingCallback
{
public:
  static CRepositoryUpdater& GetInstance();

  virtual ~CRepositoryUpdater() {}

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


  virtual void OnSettingChanged(const CSetting* setting) override;

  struct RepositoryUpdated { };

  CEventStream<RepositoryUpdated>& Events() { return m_events; }

private:
  CRepositoryUpdater();
  CRepositoryUpdater(const CRepositoryUpdater&) = delete;
  CRepositoryUpdater(CRepositoryUpdater&&) = delete;
  CRepositoryUpdater& operator=(const CRepositoryUpdater&) = delete;
  CRepositoryUpdater& operator=(CRepositoryUpdater&&) = delete;

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  virtual void OnTimeout() override;

  CCriticalSection m_criticalSection;
  CTimer m_timer;
  CEvent m_doneEvent;
  std::vector<CRepositoryUpdateJob*> m_jobs;

  CEventSource<RepositoryUpdated> m_events;
};
}
