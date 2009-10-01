#ifndef PERFORMANCESTATS_H
#define PERFORMANCESTATS_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <map>
#include <string>
#include "PlatformDefs.h"
#include "CriticalSection.h"

class PerformanceCounter
{
public:
  double     m_time;
  double     m_user;
  double     m_sys;
  __int64    m_samples;

  PerformanceCounter(double dTime=0.0, double dUser=0.0, double dSys=0.0, __int64 nSamples=1LL) :
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
