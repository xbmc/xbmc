/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include <chrono>

namespace kodi
{
namespace tools
{

//==============================================================================
/// @defgroup cpp_kodi_tools_CEndTime class CEndTime
/// @ingroup cpp_kodi_tools
/// @brief **Timeout check**\n
/// Class which makes it easy to check if a specified amount of time has passed.
///
/// This code uses the support of platform-independent chrono system introduced
/// with C++11.
///
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/tools/EndTime.h>
///
/// class ATTR_DLL_LOCAL CExample
/// {
/// public:
///   CExample()
///   {
///     TimerCall();
///   }
///
///   void TimerCall()
///   {
///     fprintf(stderr, "Hello World\n");
///     CEndTime timer(1000);
///
///     while (timer.MillisLeft())
///     {
///       if (timer.IsTimePast())
///       {
///         fprintf(stderr, "We timed out!\n");
///       }
///       std::this_thread::sleep_for(std::chrono::milliseconds(10));
///     }
///   }
///
/// };
/// ~~~~~~~~~~~~~
///
///@{
class CEndTime
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Class constructor with no time to expiry set
  ///
  inline CEndTime() = default;
  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Class constructor to set future time when timer has expired
  ///
  /// @param[in] millisecondsIntoTheFuture the time in the future we cosider this timer as expired
  ///
  inline explicit CEndTime(unsigned int millisecondsIntoTheFuture)
    : m_startTime(std::chrono::system_clock::now().time_since_epoch()),
      m_totalWaitTime(std::chrono::milliseconds(millisecondsIntoTheFuture))
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Set the time in the future we cosider this timer as expired
  ///
  /// @param[in] millisecondsIntoTheFuture the time in the future we cosider this timer as expired
  ///
  inline void Set(unsigned int millisecondsIntoTheFuture)
  {
    using namespace std::chrono;

    m_startTime = system_clock::now().time_since_epoch();
    m_totalWaitTime = milliseconds(millisecondsIntoTheFuture);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Check if the expiry time has been reached
  ///
  /// @return True if the expiry amount of time has past, false otherwise
  ///
  inline bool IsTimePast() const
  {
    using namespace std::chrono;

    // timer is infinite
    if (m_totalWaitTime.count() == std::numeric_limits<unsigned int>::max())
      return false;

    if (m_totalWaitTime.count() == 0)
      return true;
    else
      return (system_clock::now().time_since_epoch() - m_startTime) >= m_totalWaitTime;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief The amount of time left till this timer expires
  ///
  /// @return 0 if the expiry amount of time has past, the number of milliseconds remaining otherwise
  ///
  inline unsigned int MillisLeft() const
  {
    using namespace std::chrono;

    // timer is infinite
    if (m_totalWaitTime.count() == std::numeric_limits<unsigned int>::max())
      return std::numeric_limits<unsigned int>::max();

    if (m_totalWaitTime.count() == 0)
      return 0;

    auto elapsed = system_clock::now().time_since_epoch() - m_startTime;

    auto timeWaitedAlready = duration_cast<milliseconds>(elapsed).count();

    if (timeWaitedAlready >= m_totalWaitTime.count())
      return 0;

    return static_cast<unsigned int>(m_totalWaitTime.count() - timeWaitedAlready);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Consider this timer expired
  ///
  inline void SetExpired()
  {
    using namespace std::chrono;
    m_totalWaitTime = milliseconds(0);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Set this timer as never expiring
  ///
  inline void SetInfinite()
  {
    using namespace std::chrono;
    m_totalWaitTime = milliseconds(std::numeric_limits<unsigned int>::max());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Check if the timer has been set to infinite expiry
  ///
  /// @return True if the expiry has been set as infinite, false otherwise
  ///
  inline bool IsInfinite(void) const
  {
    return (m_totalWaitTime.count() == std::numeric_limits<unsigned int>::max());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Get the initial timeout value this timer had
  ///
  /// @return The initial expiry amount of time this timer had in milliseconds
  ///
  inline unsigned int GetInitialTimeoutValue(void) const
  {
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(m_totalWaitTime);
    return static_cast<unsigned int>(value.count());
  }

  //============================================================================
  /// @ingroup cpp_kodi_tools_CEndTime
  /// @brief Get the time this timer started
  ///
  /// @return The time this timer started in milliseconds since epoch
  ///
  inline uint64_t GetStartTime(void) const
  {
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(m_startTime);
    return value.count();
  }
  //----------------------------------------------------------------------------

private:
  std::chrono::system_clock::duration m_startTime;
  std::chrono::system_clock::duration m_totalWaitTime;
};

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
