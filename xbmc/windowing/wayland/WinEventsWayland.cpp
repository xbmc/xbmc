/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsWayland.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include "platform/posix/utils/FileHandle.h"

#include <exception>
#include <memory>
#include <mutex>
#include <system_error>

#include <sys/poll.h>
#include <unistd.h>
#include <wayland-client.hpp>

using namespace KODI::UTILS::POSIX;
using namespace KODI::WINDOWING::WAYLAND;

namespace
{
/**
 * Thread for processing Wayland events
 *
 * While not strictly needed, reading from the Wayland display file descriptor
 * and dispatching the resulting events is done in an extra thread here.
 * Sometime in the future, MessagePump() might be gone and then the
 * transition will be easier since this extra thread is already here.
 */
class CWinEventsWaylandThread : CThread
{
  wayland::display_t& m_display;
  // Pipe used for cancelling poll() on shutdown
  CFileHandle m_pipeRead;
  CFileHandle m_pipeWrite;

  CCriticalSection m_roundtripQueueMutex;
  std::atomic<wayland::event_queue_t*> m_roundtripQueue{nullptr};
  CEvent m_roundtripQueueEvent;

public:
  CWinEventsWaylandThread(wayland::display_t& display)
  : CThread("Wayland message pump"), m_display{display}
  {
    std::array<int, 2> fds;
    if (pipe(fds.data()) < 0)
    {
      throw std::system_error(errno, std::generic_category(), "Error creating pipe for Wayland message pump cancellation");
    }
    m_pipeRead.attach(fds[0]);
    m_pipeWrite.attach(fds[1]);
    Create();
  }

  ~CWinEventsWaylandThread() override
  {
    Stop();
    // Wait for roundtrip invocation to finish
    std::unique_lock<CCriticalSection> lock(m_roundtripQueueMutex);
  }

  void Stop()
  {
    CLog::Log(LOGDEBUG, "Stopping Wayland message pump");
    // Set m_bStop
    StopThread(false);
    InterruptPoll();
    // Now wait for actual exit
    StopThread(true);
  }

  void RoundtripQueue(wayland::event_queue_t const& queue)
  {
    wayland::event_queue_t queueCopy{queue};

    // Serialize invocations of this function - it's used very rarely and usually
    // not in parallel anyway, and doing it avoids lots of complications
    std::unique_lock<CCriticalSection> lock(m_roundtripQueueMutex);

    m_roundtripQueueEvent.Reset();
    // We can just set the value here since there is no other writer in parallel
    m_roundtripQueue.store(&queueCopy);
    // Dispatching can happen now

    // Make sure we don't wait for an event to happen on the socket
    InterruptPoll();

    if (m_bStop)
      return;

    m_roundtripQueueEvent.Wait();
  }

  wayland::display_t& GetDisplay()
  {
    return m_display;
  }

private:
  void InterruptPoll()
  {
    char c = 0;
    if (write(m_pipeWrite, &c, 1) != 1)
      throw std::runtime_error("Failed to write to wayland message pipe");
  }

  void Process() override
  {
    try
    {
      std::array<pollfd, 2> pollFds;
      pollfd& waylandPoll = pollFds[0];
      pollfd& cancelPoll = pollFds[1];
      // Wayland filedescriptor
      waylandPoll.fd = m_display.get_fd();
      waylandPoll.events = POLLIN;
      waylandPoll.revents = 0;
      // Read end of the cancellation pipe
      cancelPoll.fd = m_pipeRead;
      cancelPoll.events = POLLIN;
      cancelPoll.revents = 0;

      CLog::Log(LOGDEBUG, "Starting Wayland message pump");

      // Run until cancelled or error
      while (!m_bStop)
      {
        // dispatch() provides no way to cancel a blocked read from the socket
        // wl_display_disconnect would just close the socket, leading to problems
        // with the poll() that dispatch() uses internally - so we have to implement
        // cancellation ourselves here

        // Acquire global read intent
        wayland::read_intent readIntent = m_display.obtain_read_intent();
        m_display.flush();

        if (poll(pollFds.data(), pollFds.size(), -1) < 0)
        {
          if (errno == EINTR)
          {
            continue;
          }
          else
          {
            throw std::system_error(errno, std::generic_category(), "Error polling on Wayland socket");
          }
        }

        if (cancelPoll.revents & POLLERR || cancelPoll.revents & POLLHUP || cancelPoll.revents & POLLNVAL)
        {
          throw std::runtime_error("poll() signalled error condition on poll interruption socket");
        }

        if (waylandPoll.revents & POLLERR || waylandPoll.revents & POLLHUP || waylandPoll.revents & POLLNVAL)
        {
          throw std::runtime_error("poll() signalled error condition on Wayland socket");
        }

        // Read events and release intent; this does not block
        readIntent.read();
        // Dispatch default event queue
        m_display.dispatch_pending();

        if (auto* roundtripQueue = m_roundtripQueue.exchange(nullptr))
        {
          m_display.roundtrip_queue(*roundtripQueue);
          m_roundtripQueueEvent.Set();
        }
        if (cancelPoll.revents & POLLIN)
        {
          // Read away the char so we don't get another notification
          // Indepentent from m_roundtripQueue so there are no races
          char c;
          if (read(m_pipeRead, &c, 1) != 1)
            throw std::runtime_error("Error reading from wayland message pipe");
        }
      }

      CLog::Log(LOGDEBUG, "Wayland message pump stopped");
    }
    catch (std::exception const& e)
    {
      // FIXME CThread::OnException is very badly named and should probably go away
      // FIXME Thread exception handling is seriously broken:
      // Exceptions will be swallowed and do not terminate the program.
      // Even XbmcCommons::UncheckedException which claims to be there for just this
      // purpose does not cause termination, the log message will just be slightly different.

      // But here, going on would be meaningless, so do a hard exit
      CLog::Log(LOGFATAL, "Exception in Wayland message pump, exiting: {}", e.what());
      std::terminate();
    }

    // Wake up if someone is still waiting for roundtrip, won't happen anytime soon...
    m_roundtripQueueEvent.Set();
  }
};

std::unique_ptr<CWinEventsWaylandThread> g_WlMessagePump{nullptr};

}

void CWinEventsWayland::SetDisplay(wayland::display_t* display)
{
  if (display && !g_WlMessagePump)
  {
    // Start message processing as soon as we have a display
    g_WlMessagePump = std::make_unique<CWinEventsWaylandThread>(*display);
  }
  else if (g_WlMessagePump)
  {
    // Stop if display is set to nullptr
    g_WlMessagePump.reset();
  }
}

void CWinEventsWayland::Flush()
{
  if (g_WlMessagePump)
  {
    g_WlMessagePump->GetDisplay().flush();
  }
}

void CWinEventsWayland::RoundtripQueue(const wayland::event_queue_t& queue)
{
  if (g_WlMessagePump)
  {
    g_WlMessagePump->RoundtripQueue(queue);
  }
}

bool CWinEventsWayland::MessagePump()
{
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  // Forward any events that may have been pushed to our queue
  while (true)
  {
    XBMC_Event event;
    {
      // Scoped lock for reentrancy
      std::unique_lock<CCriticalSection> lock(m_queueMutex);

      if (m_queue.empty())
      {
        break;
      }

      // First get event and remove it from the queue, then pass it on - be aware that this
      // function must be reentrant
      event = m_queue.front();
      m_queue.pop();
    }

    if (appPort)
      appPort->OnEvent(event);
  }

  return true;
}

void CWinEventsWayland::MessagePush(XBMC_Event* ev)
{
  std::unique_lock<CCriticalSection> lock(m_queueMutex);
  m_queue.emplace(*ev);
}
