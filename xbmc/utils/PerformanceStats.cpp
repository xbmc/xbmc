//
// C++ Implementation: PerformanceStats
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "PerformanceStats.h"
#include "PerformanceSample.h"
#include "log.h"

CPerformanceStats::CPerformanceStats()
{
}


CPerformanceStats::~CPerformanceStats()
{
  std::map<std::string, PerformanceCounter*>::iterator iter = m_mapStats.begin();
  while (iter != m_mapStats.end())
  {
    delete iter->second;
    iter++;
  }
  m_mapStats.clear();
}

void CPerformanceStats::AddSample(const std::string &strStatName, const PerformanceCounter &perf)
{
  std::map<std::string, PerformanceCounter*>::iterator iter = m_mapStats.find(strStatName);
  if (iter == m_mapStats.end())
    m_mapStats[strStatName] = new PerformanceCounter(perf);
  else
  {
    iter->second->m_time += perf.m_time;
    iter->second->m_samples += perf.m_samples;
  }
}

void CPerformanceStats::AddSample(const std::string &strStatName, double dTime)
{
  AddSample(strStatName, *(new PerformanceCounter(dTime)));
}

void CPerformanceStats::DumpStats()
{
  double dError = CPerformanceSample::GetEstimatedError();
  CLog::Log(LOGINFO, "%s - estimated error: %f", __FUNCTION__, dError);
  CLog::Log(LOGINFO," %s - ignore user/sys values when sample count is low", __FUNCTION__);

  std::map<std::string, PerformanceCounter*>::iterator iter = m_mapStats.begin();
  while (iter != m_mapStats.end())
  {
    double dAvg = iter->second->m_time / (double)iter->second->m_samples;
    double dAvgUser = iter->second->m_user / (double)iter->second->m_samples;
    double dAvgSys  = iter->second->m_sys / (double)iter->second->m_samples;
    CLog::Log(LOGINFO, "%s - counter <%s>. avg duration: <%f sec>, avg user: <%f>, avg sys: <%f> (%llu samples)", 
      __FUNCTION__, iter->first.c_str(), dAvg, dAvgUser, dAvgSys, iter->second->m_samples);
    iter++;
  }
}


