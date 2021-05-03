/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <stdint.h>

class CStopWatch
{
public:
  CStopWatch() = default;
  ~CStopWatch() = default;

  /*!
    \brief Retrieve the running state of the stopwatch.

    \return True if stopwatch has been started but not stopped.
  */
  inline bool IsRunning() const
  {
    return m_isRunning;
  }

  /*!
    \brief Record start time and change state to running.
  */
  inline void StartZero()
  {
    m_startTick = std::chrono::steady_clock::now();
    m_isRunning = true;
  }

  /*!
    \brief Record start time and change state to running, only if the stopwatch is stopped.
  */
  inline void Start()
  {
    if (!m_isRunning)
      StartZero();
  }

  /*!
    \brief Record stop time and change state to not running.
  */
  inline void Stop()
  {
    if(m_isRunning)
    {
      m_stopTick = std::chrono::steady_clock::now();
      m_isRunning = false;
    }
  }

  /*!
    \brief Set the start time such that time elapsed is now zero.
  */
  void Reset()
  {
    if (m_isRunning)
      m_startTick = std::chrono::steady_clock::now();
    else
      m_startTick = m_stopTick;
  }

  /*!
    \brief  Retrieve time elapsed between the last call to Start(), StartZero()
            or Reset() and; if running, now; if stopped, the last call to Stop().

    \return Elapsed time, in seconds, as a float.
  */
  float GetElapsedSeconds() const
  {
    std::chrono::duration<float> elapsed;

    if (m_isRunning)
      elapsed = std::chrono::steady_clock::now() - m_startTick;
    else
      elapsed = m_stopTick - m_startTick;

    return elapsed.count();
  }

  /*!
    \brief  Retrieve time elapsed between the last call to Start(), StartZero()
            or Reset() and; if running, now; if stopped, the last call to Stop().

    \return Elapsed time, in milliseconds, as a float.
  */
  float GetElapsedMilliseconds() const
  {
    std::chrono::duration<float, std::milli> elapsed;

    if (m_isRunning)
      elapsed = std::chrono::steady_clock::now() - m_startTick;
    else
      elapsed = m_stopTick - m_startTick;

    return elapsed.count();
  }

private:
  std::chrono::time_point<std::chrono::steady_clock> m_startTick;
  std::chrono::time_point<std::chrono::steady_clock> m_stopTick;
  bool m_isRunning = false;
};
