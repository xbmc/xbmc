/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include "Thread.h"

#include <functional>

namespace kodi
{
namespace tools
{

//==============================================================================
/// @defgroup cpp_kodi_tools_CTimer class CTimer
/// @ingroup cpp_kodi_tools
/// @brief **Time interval management**\n
/// Class which enables a time interval to be called up by a given function or
/// class by means of a thread.
///
/// His code uses the support of platform-independent thread system introduced
/// with C++11.
///
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/tools/Timer.h>
///
/// class ATTR_DLL_LOCAL CExample
/// {
/// public:
///   CExample() : m_timer([this](){TimerCall();})
///   {
///     m_timer.Start(5000, true); // let call continuously all 5 seconds
///   }
///
///   void TimerCall()
///   {
///     fprintf(stderr, "Hello World\n");
///   }
///
/// private:
///   kodi::tools::CTimer m_timer;
/// };
/// ~~~~~~~~~~~~~
///
///@{
class CTimer : protected CThread
{
public:
  class ITimerCallback;

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Class constructor to pass individual other class as callback.
  ///
  /// @param[in] callback Child class of parent @ref ITimerCallback with
  ///                     implemented function @ref ITimerCallback::OnTimeout().
  ///
  explicit CTimer(kodi::tools::CTimer::ITimerCallback* callback)
    : CTimer(std::bind(&ITimerCallback::OnTimeout, callback))
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Class constructor to pass individual function as callback.
  ///
  /// @param[in] callback Function to pass as callback about timeout.
  ///
  /// **Callback function style:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// void TimerCallback()
  /// {
  /// }
  /// ~~~~~~~~~~~~~
  explicit CTimer(std::function<void()> const& callback) : m_callback(callback) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Class destructor.
  ///
  ~CTimer() override { Stop(true); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Start the timer by given time in milliseconds to make his call
  /// by arrive of them.
  ///
  /// If interval is activated, it calls the associated callback function
  /// continuously in the given interval.
  ///
  /// @param[in] timeout Timeout in milliseconds
  /// @param[in] interval [opt] To run continuously if true, false only one time
  ///                     and default
  /// @return True if successfully done, false if not (callback missing,
  ///         timeout = 0 or was already running.
  ///
  bool Start(uint64_t timeout, bool interval = false)
  {
    using namespace std::chrono;

    if (m_callback == nullptr || timeout == 0 || IsRunning())
      return false;

    m_timeout = milliseconds(timeout);
    m_interval = interval;

    CreateThread();
    return true;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Stop the timer if it is active.
  ///
  /// @param[in] wait [opt] Wait until timer is stopped, false is default and
  ///                 call unblocked
  /// @return True if timer was active and was stopped, false if already was
  ///         stopped.
  ///
  bool Stop(bool wait = false)
  {
    if (!IsRunning())
      return false;

    m_threadStop = true;
    m_eventTimeout.notify_all();
    StopThread(wait);

    return true;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Restart timer complete by stop and restart his thread again.
  ///
  /// @note Restart only possible as long the timer was running and not done his
  /// work.
  ///
  /// @return True if start was successfully done, on error, or if was already
  ///         finished returned as false
  ///
  bool Restart()
  {
    using namespace std::chrono;

    if (!IsRunning())
      return false;

    Stop(true);
    return Start(duration_cast<milliseconds>(m_timeout).count(), m_interval);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Restart the timer with new timeout without touch of his thread.
  ///
  /// @param[in] timeout Time as milliseconds to wait for next call
  ///
  void RestartAsync(uint64_t timeout)
  {
    using namespace std::chrono;

    m_timeout = milliseconds(timeout);
    const auto now = system_clock::now();
    m_endTime = now.time_since_epoch() + m_timeout;
    m_eventTimeout.notify_all();
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Check timer is still active to wait for next call.
  ///
  /// @return True if active, false if all his work done and no more running
  ///
  bool IsRunning() const { return CThread::IsRunning(); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Get elapsed time as floating point of timer as seconds.
  ///
  /// @return Elapsed time
  ///
  float GetElapsedSeconds() const { return GetElapsedMilliseconds() / 1000.0f; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief Get elapsed time as floating point of timer as milliseconds.
  ///
  /// @return Elapsed time
  ///
  float GetElapsedMilliseconds() const
  {
    using namespace std::chrono;

    if (!IsRunning())
      return 0.0f;

    const auto now = system_clock::now();
    return static_cast<float>(
        duration_cast<milliseconds>(now.time_since_epoch() - (m_endTime - m_timeout)).count());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_tools_CTimer_CB class ITimerCallback
  /// @ingroup cpp_kodi_tools_CTimer
  /// @brief **Callback class of timer**\n
  /// To give on constructor by @ref CTimer(kodi::tools::CTimer::ITimerCallback* callback)
  ///
  class ITimerCallback
  {
  public:
    //==========================================================================
    /// @ingroup cpp_kodi_tools_CTimer_CB
    /// @brief Class destructor.
    ///
    virtual ~ITimerCallback() = default;
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_tools_CTimer_CB
    /// @brief Callback function to implement if constructor @ref CTimer(kodi::tools::CTimer::ITimerCallback* callback)
    /// is used and this as parent on related class
    ///
    /// ----------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/tools/Timer.h>
    ///
    /// class CExample : public kodi::tools::CTimer,
    ///                  private kodi::tools::CTimer::ITimerCallback
    /// {
    /// public:
    ///   CExample() : kodi::tools::CTimer(this)
    ///   {
    ///   }
    ///
    ///   void OnTimeout() override
    ///   {
    ///     // Some work
    ///   }
    /// };
    ///
    /// ~~~~~~~~~~~~~
    ///
    virtual void OnTimeout() = 0;
    //--------------------------------------------------------------------------
  };
  //----------------------------------------------------------------------------

protected:
  void Process() override
  {
    using namespace std::chrono;

    while (!m_threadStop)
    {
      auto currentTime = system_clock::now();
      m_endTime = currentTime.time_since_epoch() + m_timeout;

      // wait the necessary time
      std::mutex mutex;
      std::unique_lock<std::mutex> lock(mutex);
      const auto waitTime = duration_cast<milliseconds>(m_endTime - currentTime.time_since_epoch());
      if (m_eventTimeout.wait_for(lock, waitTime) == std::cv_status::timeout)
      {
        currentTime = system_clock::now();
        if (m_endTime.count() <= currentTime.time_since_epoch().count())
        {
          // execute OnTimeout() callback
          m_callback();

          // continue if this is an interval timer, or if it was restarted during callback
          if (!m_interval && m_endTime.count() <= currentTime.time_since_epoch().count())
            break;
        }
      }
    }
  }

private:
  bool m_interval = false;
  std::function<void()> m_callback;
  std::chrono::system_clock::duration m_timeout;
  std::chrono::system_clock::duration m_endTime;
  std::condition_variable_any m_eventTimeout;
};
///@}
//------------------------------------------------------------------------------

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
