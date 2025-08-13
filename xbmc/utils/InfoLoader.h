/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/IJobCallback.h"

#include <atomic>
#include <string>

class CInfoLoader : public IJobCallback
{
public:
  explicit CInfoLoader(unsigned int timeToRefresh = 5 * 60 * 1000);
  ~CInfoLoader() override;

  std::string GetInfo(int info);
  void Refresh();
  bool IsUpdating() const { return m_busy; }

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
protected:
  bool RefreshIfNeeded();
  virtual CJob *GetJob() const=0;
  virtual std::string TranslateInfo(int info) const;
  virtual std::string BusyInfo(int info) const;
private:
  unsigned int m_refreshTime;
  unsigned int m_timeToRefresh;
  std::atomic<bool> m_busy{false};
};
