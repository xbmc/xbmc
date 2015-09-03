/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "PerformanceStats.h"
#include "PerformanceSample.h"
#include "log.h"

using namespace std;

CPerformanceStats::CPerformanceStats()
{
}


CPerformanceStats::~CPerformanceStats()
{
  map<string, PerformanceCounter*>::iterator iter = m_mapStats.begin();
  while (iter != m_mapStats.end())
  {
    delete iter->second;
    ++iter;
  }
  m_mapStats.clear();
}

void CPerformanceStats::AddSample(const string &strStatName, const PerformanceCounter &perf)
{
  map<string, PerformanceCounter*>::iterator iter = m_mapStats.find(strStatName);
  if (iter == m_mapStats.end())
    m_mapStats[strStatName] = new PerformanceCounter(perf);
  else
  {
    iter->second->m_time += perf.m_time;
    iter->second->m_samples += perf.m_samples;
  }
}

void CPerformanceStats::AddSample(const string &strStatName, double dTime)
{
  AddSample(strStatName, PerformanceCounter(dTime));
}

void CPerformanceStats::DumpStats()
{
  double dError = CPerformanceSample::GetEstimatedError();
  CLog::Log(LOGINFO, "%s - estimated error: %f", __FUNCTION__, dError);
  CLog::Log(LOGINFO, "%s - ignore user/sys values when sample count is low", __FUNCTION__);

  map<string, PerformanceCounter*>::iterator iter = m_mapStats.begin();
  while (iter != m_mapStats.end())
  {
    double dAvg = iter->second->m_time / (double)iter->second->m_samples;
    double dAvgUser = iter->second->m_user / (double)iter->second->m_samples;
    double dAvgSys  = iter->second->m_sys / (double)iter->second->m_samples;
    CLog::Log(LOGINFO, "%s - counter <%s>. avg duration: <%f sec>, avg user: <%f>, avg sys: <%f> (%" PRIu64" samples)",
      __FUNCTION__, iter->first.c_str(), dAvg, dAvgUser, dAvgSys, iter->second->m_samples);
    ++iter;
  }
}


