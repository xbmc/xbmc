/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

#include "utils/log.h"

#include "PollThread.h"

namespace xwe = xbmc::wayland::events;

xwe::PollThread::PollThread(const Dispatch &dispatch,
                            const Dispatch &beforePoll,
                            int fd) :
  CThread("wayland-event-thread"),
  m_dispatch(dispatch),
  m_beforePoll(beforePoll),
  m_fd(fd)
{
  /* Create wakeup pipe. We will poll on the read end of this. If
   * there are any events, it means its time to shut down */
  if (pipe2(m_wakeupPipe, O_CLOEXEC) == -1)
  {
    std::stringstream ss;
    ss << strerror(errno);
    throw std::runtime_error(ss.str());
  }
  
  /* Create poll thread */
  Create();
}

xwe::PollThread::~PollThread()
{
  /* Send a message to the reader thread that its time to shut down */
  if (write(m_wakeupPipe[1], (const void *) "q", 1) == -1)
    CLog::Log(LOGERROR, "%s: Failed to write to shutdown wakeup pipe. "
                        "Application may hang. Reason was: %s",
              __FUNCTION__, strerror(errno));

  /* Close the write end, as we no longer need it */
  if (close(m_wakeupPipe[1]) == -1)
    CLog::Log(LOGERROR, "%s: Failed to close shutdown pipe write end. "
                      "Leak may occurr. Reason was: %s",
            __FUNCTION__, strerror(errno));

  /* The destructor for CThread will cause it to join */
}

namespace
{
bool Ready(int revents)
{
  if (revents & (POLLHUP | POLLERR))
    CLog::Log(LOGERROR, "%s: Error on fd. Reason was: %s",
              __FUNCTION__, strerror(errno));
  
  return revents & POLLIN;
}
}

void
xwe::PollThread::Process()
{
  static const unsigned int MonitoredFdIndex = 0;
  static const unsigned int ShutdownFdIndex = 1;
  
  while (1)
  {
    struct pollfd pfd[2] =
    {
      { m_fd, POLLIN | POLLHUP | POLLERR, 0 },
      { m_wakeupPipe[0], POLLIN | POLLHUP | POLLERR, 0 }
    };
    
    m_beforePoll();
    
    /* Sleep forever until something happens */
    if (poll(pfd, 2, -1) == -1)
      CLog::Log(LOGERROR, "%s: Poll failed. Reason was: %s",
                __FUNCTION__, strerror(errno));
    
    /* Check the shutdown pipe first as we might need to
     * shutdown */
    if (Ready(pfd[ShutdownFdIndex].revents))
    {
      if (close(m_wakeupPipe[0]) == -1)
        CLog::Log(LOGERROR, "%s: Failed to close shutdown pipe read end. "
                            "Leak may occurr. Reason was: %s",
                  __FUNCTION__, strerror(errno));
      return;
    }
    else if (Ready(pfd[MonitoredFdIndex].revents))
      m_dispatch();
  }
}
