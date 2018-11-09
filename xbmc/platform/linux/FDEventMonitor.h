/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/GlobalsHandling.h"
#include <map>
#include <sys/epoll.h>
#include <vector>

/**
 * Monitor a file descriptor with callback on poll() events.
 */
class CFDEventMonitor : private CThread
{
public:

  typedef void (*EventCallback)(int id, int fd, short revents, void *data);

  struct MonitoredFD
  {
    int fd = -1; /**< File descriptor to be monitored */
    short events = 0; /**< Events to be monitored (see poll(2)) */

    EventCallback callback = nullptr; /** Callback to be called on events */
    void *callbackData = nullptr; /** data parameter for EventCallback */

    MonitoredFD(int fd_, short events_, EventCallback callback_, void *callbackData_) :
      fd(fd_), events(events_), callback(callback_), callbackData(callbackData_) {}
    MonitoredFD() = default;
  };

  CFDEventMonitor();
  ~CFDEventMonitor() override;

  void AddFD(const MonitoredFD& monitoredFD, int& id);
  void AddFDs(const std::vector<MonitoredFD>& monitoredFDs, std::vector<int>& ids);

  void RemoveFD(int id);
  void RemoveFDs(const std::vector<int>& ids);

protected:
  void Process() override;

private:
  void AddFDLocked(const MonitoredFD& monitoredFD, int& id);

  void AddPollDesc(int id, int fd, short events);
  void UpdatePollDescs();

  void StartMonitoring();
  void InterruptPoll();

  std::map<int, MonitoredFD> m_monitoredFDs;

  /* these are kept synchronized */
  std::vector<int> m_monitoredFDbyPollDescs;
  std::vector<struct pollfd> m_pollDescs;

  int m_nextID = 0;
  int m_wakeupfd = -1;

  CCriticalSection m_mutex;
  CCriticalSection m_pollMutex;
};

XBMC_GLOBAL_REF(CFDEventMonitor, g_fdEventMonitor);
#define g_fdEventMonitor XBMC_GLOBAL_USE(CFDEventMonitor)
