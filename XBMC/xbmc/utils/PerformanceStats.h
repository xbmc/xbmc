//
// C++ Interface: PerformanceStats
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PERFORMANCESTATS_H
#define PERFORMANCESTATS_H

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
