/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

class CStopWatch
{
public:
  explicit CStopWatch(bool useFrameTime=false);
  ~CStopWatch();

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
    m_startTick = GetTicks();
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
      m_stopTick = GetTicks();
      m_isRunning = false;
    }
  }

  /*!
    \brief Set the start time such that time elapsed is now zero.
  */
  void Reset()
  {
    if (m_isRunning)
      m_startTick = GetTicks();
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
    int64_t totalTicks = (m_isRunning ? GetTicks() : m_stopTick) - m_startTick;
    return (float)totalTicks * m_timerPeriod;
  }

  /*!
    \brief  Retrieve time elapsed between the last call to Start(), StartZero()
            or Reset() and; if running, now; if stopped, the last call to Stop().

    \return Elapsed time, in milliseconds, as a float.
  */
  float GetElapsedMilliseconds() const
  {
    return GetElapsedSeconds() * 1000.0f;
  }

private:
  int64_t GetTicks() const;
  float m_timerPeriod;        // to save division in GetElapsed...()
  int64_t m_startTick;
  int64_t m_stopTick;
  bool m_isRunning;
  bool m_useFrameTime;
};
