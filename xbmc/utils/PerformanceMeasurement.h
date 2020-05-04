/*
 *  Copyright (C) 2013-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <algorithm>
#include <chrono>

template<class TClock = std::chrono::high_resolution_clock>
class CPerformanceMeasurement
{
public:
  using Duration = typename TClock::duration;

  CPerformanceMeasurement()
  {
    if (!m_initialized)
    {
      m_overhead = MeasureOverhead();
      m_initialized = true;
    }

    Restart();
  }

  ~CPerformanceMeasurement() { Stop(); }

  void Stop()
  {
    const auto now = TClock::now();
    m_duration =
        std::max(Duration(0), std::chrono::duration_cast<Duration>(now - m_start) - m_overhead);
  }

  void Restart() { m_start = TClock::now(); }

  Duration GetDuration() const { return m_duration; }

  double GetDurationInSeconds() const { return GetDurationInSeconds(m_duration); }

private:
  static double GetDurationInSeconds(const typename TClock::duration& duration)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
  }

  Duration MeasureOverhead()
  {
    static constexpr size_t LoopCount = 1000;
    const typename TClock::time_point overheadStart = TClock::now();
    for (size_t i = 0; i < LoopCount; ++i)
    {
      Restart();
      Stop();
    }

    const Duration overhead = std::chrono::duration_cast<Duration>(TClock::now() - overheadStart);
    return overhead / LoopCount;
  }

  static Duration m_overhead;
  static bool m_initialized;

  typename TClock::time_point m_start;
  Duration m_duration;
};

template<class TClock>
typename CPerformanceMeasurement<TClock>::Duration CPerformanceMeasurement<TClock>::m_overhead =
    typename CPerformanceMeasurement<TClock>::Duration(0);

template<class TClock>
bool CPerformanceMeasurement<TClock>::m_initialized = false;
