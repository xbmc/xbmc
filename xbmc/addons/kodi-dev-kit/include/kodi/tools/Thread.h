/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include "../General.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

namespace kodi
{
namespace tools
{

class CThread
{
public:
  CThread() : m_threadStop(false) {}

  virtual ~CThread()
  {
    StopThread();
    if (m_thread != nullptr)
    {
      m_thread->detach();
      delete m_thread;
    }
  }

  bool IsAutoDelete() const { return m_autoDelete; }

  bool IsCurrentThread() const { return m_threadId == std::this_thread::get_id(); }

  bool IsRunning() const
  {
    if (m_thread != nullptr)
    {
      // it's possible that the thread exited on it's own without a call to StopThread. If so then
      // the promise should be fulfilled.
      std::future_status stat = m_future.wait_for(std::chrono::milliseconds(0));
      // a status of 'ready' means the future contains the value so the thread has exited
      // since the thread can't exit without setting the future.
      if (stat == std::future_status::ready) // this is an indication the thread has exited.
        return false;
      return true; // otherwise the thread is still active.
    }
    else
      return false;
  }

  void CreateThread(bool autoDelete = false)
  {
    if (m_thread != nullptr)
    {
      // if the thread exited on it's own, without a call to StopThread, then we can get here
      // incorrectly. We should be able to determine this by checking the promise.
      std::future_status stat = m_future.wait_for(std::chrono::milliseconds(0));
      // a status of 'ready' means the future contains the value so the thread has exited
      // since the thread can't exit without setting the future.
      if (stat == std::future_status::ready) // this is an indication the thread has exited.
        StopThread(true); // so let's just clean up
      else
      { // otherwise we have a problem.
        kodi::Log(ADDON_LOG_FATAL, "%s - fatal error creating thread - old thread id not null",
                  __func__);
        exit(1);
      }
    }

    m_autoDelete = autoDelete;
    m_threadStop = false;
    m_startEvent.notify_all();
    m_stopEvent.notify_all();

    std::promise<bool> prom;
    m_future = prom.get_future();

    {
      // The std::thread internals must be set prior to the lambda doing
      //   any work. This will cause the lambda to wait until m_thread
      //   is fully initialized. Interestingly, using a std::atomic doesn't
      //   have the appropriate memory barrier behavior to accomplish the
      //   same thing so a full system mutex needs to be used.
      std::unique_lock<std::recursive_mutex> lock(m_threadMutex);
      m_thread = new std::thread(
          [](CThread* thread, std::promise<bool> promise) {
            try
            {
              {
                // Wait for the pThread->m_thread internals to be set. Otherwise we could
                // get to a place where we're reading, say, the thread id inside this
                // lambda's call stack prior to the thread that kicked off this lambda
                // having it set. Once this lock is released, the CThread::Create function
                // that kicked this off is done so everything should be set.
                std::unique_lock<std::recursive_mutex> lock(thread->m_threadMutex);
              }

              thread->m_threadId = std::this_thread::get_id();
              std::stringstream ss;
              ss << thread->m_threadId;
              std::string id = ss.str();
              bool autodelete = thread->m_autoDelete;

              kodi::Log(ADDON_LOG_DEBUG, "Thread %s start, auto delete: %s", id.c_str(),
                        (autodelete ? "true" : "false"));

              thread->m_running = true;
              thread->m_startEvent.notify_one();

              thread->Process();

              if (autodelete)
              {
                kodi::Log(ADDON_LOG_DEBUG, "Thread %s terminating (autodelete)", id.c_str());
                delete thread;
                thread = nullptr;
              }
              else
                kodi::Log(ADDON_LOG_DEBUG, "Thread %s terminating", id.c_str());
            }
            catch (const std::exception& e)
            {
              kodi::Log(ADDON_LOG_DEBUG, "Thread Terminating with Exception: %s", e.what());
            }
            catch (...)
            {
              kodi::Log(ADDON_LOG_DEBUG, "Thread Terminating with Exception");
            }

            promise.set_value(true);
          },
          this, std::move(prom));

      m_startEvent.wait(lock);
    }
  }

  void StopThread(bool wait = true)
  {
    std::unique_lock<std::recursive_mutex> lock(m_threadMutex);

    if (m_threadStop)
      return;

    if (!m_running)
      m_startEvent.wait(lock);
    m_running = false;
    m_threadStop = true;
    m_stopEvent.notify_one();

    std::thread* lthread = m_thread;
    if (lthread != nullptr && wait && !IsCurrentThread())
    {
      lock.unlock();
      if (lthread->joinable())
        lthread->join();
      delete m_thread;
      m_thread = nullptr;
      m_threadId = std::thread::id();
    }
  }

  void Sleep(uint32_t milliseconds)
  {
    if (milliseconds > 10 && IsCurrentThread())
    {
      std::unique_lock<std::recursive_mutex> lock(m_threadMutex);
      m_stopEvent.wait_for(lock, std::chrono::milliseconds(milliseconds));
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
  }

  bool Join(unsigned int milliseconds)
  {
    std::unique_lock<std::recursive_mutex> lock(m_threadMutex);
    std::thread* lthread = m_thread;
    if (lthread != nullptr)
    {
      if (IsCurrentThread())
        return false;

      {
        m_threadMutex.unlock(); // don't hold the thread lock while we're waiting
        std::future_status stat = m_future.wait_for(std::chrono::milliseconds(milliseconds));
        if (stat != std::future_status::ready)
          return false;
        m_threadMutex.lock();
      }

      // it's possible it's already joined since we released the lock above.
      if (lthread->joinable())
        m_thread->join();
      return true;
    }
    else
      return false;
  }

protected:
  virtual void Process() = 0;

  std::atomic<bool> m_threadStop;

private:
  bool m_autoDelete = false;
  bool m_running = false;
  std::condition_variable_any m_stopEvent;
  std::condition_variable_any m_startEvent;
  std::recursive_mutex m_threadMutex;
  std::thread::id m_threadId;
  std::thread* m_thread = nullptr;
  std::future<bool> m_future;
};

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
