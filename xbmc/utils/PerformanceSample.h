#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_POSIX
#include "linux/PlatformDefs.h"
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#elif TARGET_WINDOWS
#include "platform/win32/PlatformDefs.h"
#endif

#include <string>

#ifndef NO_PERFORMANCE_MEASURE
#define MEASURE_FUNCTION CPerformanceSample aSample(__FUNCTION__,true);
#define BEGIN_MEASURE_BLOCK(n) { CPerformanceSample aSample(n,true);
#define END_MEASURE_BLOCK }
#else
#define MEASURE_FUNCTION
#define BEGIN_MEASURE_BLOCK(n)
#define END_MEASURE_BLOCK
#endif

class CPerformanceSample
{
public:
  CPerformanceSample(const std::string &statName, bool bCheckWhenDone=true);
  virtual ~CPerformanceSample();

  void Reset();
  void CheckPoint(); // will add a sample to stats and restart counting.

  static double GetEstimatedError();

protected:
  std::string m_statName;
  bool m_bCheckWhenDone;

#ifdef TARGET_POSIX
  struct rusage m_usage;
#endif

  int64_t m_tmStart;
  static int64_t m_tmFreq;
};

