#ifndef PERFORMANCESTATS_H
#define PERFORMANCESTATS_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include <map>
#include <string>
#include "PlatformDefs.h"
#include "threads/CriticalSection.h"

class PerformanceCounter
{
public:
  double     m_time;
  double     m_user;
  double     m_sys;
  int64_t    m_samples;

  PerformanceCounter(double dTime=0.0, double dUser=0.0, double dSys=0.0, int64_t nSamples=1LL) :
     m_time(dTime), m_user(dUser), m_sys(dSys), m_samples(nSamples) { }
  virtual ~PerformanceCounter() { }
};

/**
*/
class CPerformanceStats{
public:
  CPerformanceStats();
  virtual ~CPerformanceStats();

  void AddSample(const std::string &strStatName, const PerformanceCounter &perf);
  void AddSample(const std::string &strStatName, double dTime);
  void DumpStats();

protected:
  CCriticalSection                            m_lock;
  std::map<std::string, PerformanceCounter*>  m_mapStats;
};

#endif
