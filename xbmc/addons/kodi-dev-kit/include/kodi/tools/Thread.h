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

//==============================================================================
/// @defgroup cpp_kodi_tools_CThread class CThread
/// @ingroup cpp_kodi_tools
/// @brief **Helper class to represent threads of execution**\n
/// An execution thread is a sequence of instructions that can run concurrently
/// with other such sequences in multithreaded environments while sharing the
/// same address space.
///
/// Is intended to reduce any code work of C++ on addons and to have them faster
/// to use.
///
/// His code uses the support of platform-independent thread system introduced
/// with C++11.
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/tools/Thread.h>
/// #include <kodi/AddonBase.h>
///
/// class ATTR_DLL_LOCAL CTestAddon
///   : public kodi::addon::CAddonBase,
///     public kodi::tools::CThread
/// {
/// public:
///   CTestAddon() = default;
///
///   ADDON_STATUS Create() override;
///
///   void Process() override;
/// };
///
/// ADDON_STATUS CTestAddon::Create()
/// {
///   kodi::Log(ADDON_LOG_INFO, "Starting thread");
///   CreateThread();
///
///   Sleep(4000);
///
///   kodi::Log(ADDON_LOG_INFO, "Stopping thread");
///   // This added as example and also becomes stopped by class destructor
///   StopThread();
///
///   return ADDON_STATUS_OK;
/// }
///
/// void CTestAddon::Process()
/// {
///   kodi::Log(ADDON_LOG_INFO, "Thread started");
///
///   while (!m_threadStop)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Hello World");
///     Sleep(1000);
///   }
///
///   kodi::Log(ADDON_LOG_INFO, "Thread ended");
/// }
///
/// ADDONCREATOR(CTestAddon)
/// ~~~~~~~~~~~~~
///
///@{
class CThread
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Class constructor.
  ///
  CThread() : m_threadStop(false) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Class destructor.
  ///
  virtual ~CThread()
  {
    StopThread();
    if (m_thread != nullptr)
    {
      m_thread->detach();
      delete m_thread;
    }
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Check auto delete is enabled on this thread class.
  ///
  /// @return true if auto delete is used, false otherwise
  ///
  bool IsAutoDelete() const { return m_autoDelete; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Check caller is on this running thread.
  ///
  /// @return true if called from thread inside the class, false if from another
  ///         thread
  ///
  bool IsCurrentThread() const { return m_threadId == std::this_thread::get_id(); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Check thread inside this class is running and active.
  ///
  /// @note This function should be used from outside and not within process to
  /// check thread is active. Use use atomic bool @ref m_threadStop for this.
  ///
  /// @return true if running, false if not
  ///
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Create a new thread defined by this class on child.
  ///
  /// This starts then @ref Process() where is available on the child by addon.
  ///
  /// @param[in] autoDelete To set thread to delete itself after end, default is
  ///                       false
  ///
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
          [](CThread* thread, std::promise<bool> promise)
          {
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Stop a running thread.
  ///
  /// @param[in] wait As true (default) to wait until thread is finished and
  ///                 stopped, as false the function return directly and thread
  ///                 becomes independently stopped.
  ///
  void StopThread(bool wait = true)
  {
    std::unique_lock<std::recursive_mutex> lock(m_threadMutex);

    if (m_threadStop)
      return;

    if (m_thread && !m_running)
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Thread sleep with given amount of milliseconds.
  ///
  /// This makes a sleep in the thread with a given time value. If it is called
  /// within the process itself, it is also checked whether the thread is
  /// terminated and the sleep process is thereby interrupted.
  ///
  /// If the external point calls this, only a regular sleep is used, which runs
  /// through completely.
  ///
  /// @param[in] milliseconds Time to sleep
  ///
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief The function returns when the thread execution has completed or
  /// timing is reached in milliseconds beforehand
  ///
  /// This synchronizes the moment this function returns with the completion of
  /// all operations on the thread.
  ///
  /// @param[in] milliseconds Time to wait for join
  ///
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
  //----------------------------------------------------------------------------

protected:
  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief The function to be added by the addon as a child to carry out the
  /// process thread.
  ///
  /// Use @ref m_threadStop to check about active of thread and want stopped from
  /// external place.
  ///
  /// @note This function is necessary and must be implemented by the addon.
  ///
  virtual void Process() = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CThread
  /// @brief Atomic bool to indicate thread is active.
  ///
  /// This should be used in @ref Process() to check the activity of the thread and,
  /// if true, to terminate the process.
  ///
  /// - <b>`false`</b>: Thread active and should be run
  /// - <b>`true`</b>: Thread ends and should be stopped
  ///
  std::atomic<bool> m_threadStop;
  //----------------------------------------------------------------------------

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
///@}
//------------------------------------------------------------------------------

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
