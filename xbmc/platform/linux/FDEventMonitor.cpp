/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FDEventMonitor.h"

#include "threads/SingleLock.h"
#include "utils/log.h"

#include <errno.h>
#include <mutex>

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "PlatformDefs.h"

CFDEventMonitor::CFDEventMonitor() :
  CThread("FDEventMonitor")
{
}

CFDEventMonitor::~CFDEventMonitor()
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  InterruptPoll();

  if (m_wakeupfd >= 0)
  {
    /* sets m_bStop */
    StopThread(false);

    /* wake up the poll() call */
    eventfd_write(m_wakeupfd, 1);

    /* Wait for the thread to stop */
    {
      CSingleExit exit(m_mutex);
      StopThread(true);
    }

    close(m_wakeupfd);
  }
}

void CFDEventMonitor::AddFD(const MonitoredFD& monitoredFD, int& id)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  InterruptPoll();

  AddFDLocked(monitoredFD, id);

  StartMonitoring();
}

void CFDEventMonitor::AddFDs(const std::vector<MonitoredFD>& monitoredFDs,
                             std::vector<int>& ids)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  InterruptPoll();

  for (unsigned int i = 0; i < monitoredFDs.size(); ++i)
  {
    int id;
    AddFDLocked(monitoredFDs[i], id);
    ids.push_back(id);
  }

  StartMonitoring();
}

void CFDEventMonitor::RemoveFD(int id)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  InterruptPoll();

  if (m_monitoredFDs.erase(id) != 1)
  {
    CLog::Log(LOGERROR, "CFDEventMonitor::RemoveFD - Tried to remove non-existing monitoredFD {}",
              id);
  }

  UpdatePollDescs();
}

void CFDEventMonitor::RemoveFDs(const std::vector<int>& ids)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);
  InterruptPoll();

  for (unsigned int i = 0; i < ids.size(); ++i)
  {
    if (m_monitoredFDs.erase(ids[i]) != 1)
    {
      CLog::Log(LOGERROR,
                "CFDEventMonitor::RemoveFDs - Tried to remove non-existing monitoredFD {} while "
                "removing {} FDs",
                ids[i], (unsigned)ids.size());
    }
  }

  UpdatePollDescs();
}

void CFDEventMonitor::Process()
{
  eventfd_t dummy;

  while (!m_bStop)
  {
    std::unique_lock<CCriticalSection> lock(m_mutex);
    std::unique_lock<CCriticalSection> pollLock(m_pollMutex);

    /*
     * Leave the main mutex here to allow another thread to
     * lock it while we are in poll().
     * By then calling InterruptPoll() the other thread can
     * wake up poll and wait for the processing to pause at
     * the above lock(m_mutex).
     */
    lock.unlock();

    int err = poll(&m_pollDescs[0], m_pollDescs.size(), -1);

    if (err < 0 && errno != EINTR)
    {
      CLog::Log(LOGERROR, "CFDEventMonitor::Process - poll() failed, error {}, stopping monitoring",
                errno);
      StopThread(false);
    }

    // Something woke us up - either there is data available or we are being
    // paused/stopped via m_wakeupfd.

    for (unsigned int i = 0; i < m_pollDescs.size(); ++i)
    {
      struct pollfd& pollDesc = m_pollDescs[i];
      int id = m_monitoredFDbyPollDescs[i];
      const MonitoredFD& monitoredFD = m_monitoredFDs[id];

      if (pollDesc.revents)
      {
        if (monitoredFD.callback)
        {
          monitoredFD.callback(id, pollDesc.fd, pollDesc.revents,
                               monitoredFD.callbackData);
        }

        if (pollDesc.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          CLog::Log(LOGERROR,
                    "CFDEventMonitor::Process - polled fd {} got revents 0x{:x}, removing it",
                    pollDesc.fd, pollDesc.revents);

          /* Probably would be nice to inform our caller that their FD was
           * dropped, but oh well... */
          m_monitoredFDs.erase(id);
          UpdatePollDescs();
        }

        pollDesc.revents = 0;
      }
    }

    /* flush wakeup fd */
    eventfd_read(m_wakeupfd, &dummy);

  }
}

void CFDEventMonitor::AddFDLocked(const MonitoredFD& monitoredFD, int& id)
{
  id = m_nextID;

  while (m_monitoredFDs.count(id))
  {
    ++id;
  }
  m_nextID = id + 1;

  m_monitoredFDs[id] = monitoredFD;

  AddPollDesc(id, monitoredFD.fd, monitoredFD.events);
}

void CFDEventMonitor::AddPollDesc(int id, int fd, short events)
{
  struct pollfd newPollFD;
  newPollFD.fd = fd;
  newPollFD.events = events;
  newPollFD.revents = 0;

  m_pollDescs.push_back(newPollFD);
  m_monitoredFDbyPollDescs.push_back(id);
}

void CFDEventMonitor::UpdatePollDescs()
{
  m_monitoredFDbyPollDescs.clear();
  m_pollDescs.clear();

  for (const auto& it : m_monitoredFDs)
  {
    AddPollDesc(it.first, it.second.fd, it.second.events);
  }
}

void CFDEventMonitor::StartMonitoring()
{
  if (!IsRunning())
  {
    /* Start the monitoring thread */

    m_wakeupfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (m_wakeupfd < 0)
    {
      CLog::Log(LOGERROR, "CFDEventMonitor::StartMonitoring - Failed to create eventfd, error {}",
                errno);
      return;
    }

    /* Add wakeup fd to the fd list */
    int id;
    AddFDLocked(MonitoredFD(m_wakeupfd, POLLIN, NULL, NULL), id);

    Create(false);
  }
}

void CFDEventMonitor::InterruptPoll()
{
  if (m_wakeupfd >= 0)
  {
    eventfd_write(m_wakeupfd, 1);
    /* wait for the poll() result handling (if any) to end */
    std::unique_lock<CCriticalSection> pollLock(m_pollMutex);
  }
}
