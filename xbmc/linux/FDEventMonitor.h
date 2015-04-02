#pragma once
/*
 *      Copyright (C) 2014 Team Kodi
 *      http://xbmc.org
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#include <vector>
#include <map>

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include "utils/GlobalsHandling.h"

/**
 * Monitor a file descriptor with callback on poll() events.
 */
class CFDEventMonitor : private CThread
{
public:

  typedef void (*EventCallback)(int id, int fd, short revents, void *data);

  struct MonitoredFD
  {
    int fd; /**< File descriptor to be monitored */
    short events; /**< Events to be monitored (see poll(2)) */

    EventCallback callback; /** Callback to be called on events */
    void *callbackData; /** data parameter for EventCallback */

    MonitoredFD(int fd_, short events_, EventCallback callback_, void *callbackData_) :
      fd(fd_), events(events_), callback(callback_), callbackData(callbackData_) {}
    MonitoredFD() : fd(-1), events(0), callback(NULL), callbackData(NULL) {}
  };

  CFDEventMonitor();
  ~CFDEventMonitor();

  void AddFD(const MonitoredFD& monitoredFD, int& id);
  void AddFDs(const std::vector<MonitoredFD>& monitoredFDs, std::vector<int>& ids);

  void RemoveFD(int id);
  void RemoveFDs(const std::vector<int>& ids);

protected:
  virtual void Process();

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

  int m_nextID;
  int m_wakeupfd;

  CCriticalSection m_mutex;
  CCriticalSection m_pollMutex;
};

XBMC_GLOBAL_REF(CFDEventMonitor, g_fdEventMonitor);
#define g_fdEventMonitor XBMC_GLOBAL_USE(CFDEventMonitor)
