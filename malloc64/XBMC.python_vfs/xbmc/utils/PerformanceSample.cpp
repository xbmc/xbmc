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

#include "PerformanceSample.h"

#ifdef _LINUX
#include "linux/PlatformInclude.h"
#include "linux/XTimeUtils.h"
#endif

#include <SDL/SDL.h>
#include "Application.h"
#include "log.h"

LARGE_INTEGER CPerformanceSample::m_tmFreq;

CPerformanceSample::CPerformanceSample(const std::string &statName, bool bCheckWhenDone)
{
  m_statName = statName;
  m_bCheckWhenDone = bCheckWhenDone;
  if (m_tmFreq.QuadPart == 0LL)
    QueryPerformanceFrequency(&m_tmFreq);

  Reset();
}

CPerformanceSample::~CPerformanceSample()
{
  if (m_bCheckWhenDone)
    CheckPoint();
}

void CPerformanceSample::Reset()
{
  QueryPerformanceCounter(&m_tmStart);
#ifdef _LINUX
  if (getrusage(RUSAGE_SELF, &m_usage) == -1)
    CLog::Log(LOGERROR,"error %d in getrusage", errno);
#endif
}

void CPerformanceSample::CheckPoint()
{
  LARGE_INTEGER tmNow;
  QueryPerformanceCounter(&tmNow);
  double elapsed = (double)(tmNow.QuadPart - m_tmStart.QuadPart) / (double)m_tmFreq.QuadPart;

  double dUser=0.0, dSys=0.0;
#ifdef _LINUX
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == -1)
    CLog::Log(LOGERROR,"error %d in getrusage", errno);
  else
  {
    dUser = ( ((double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0) -
              ((double)m_usage.ru_utime.tv_sec + (double)m_usage.ru_utime.tv_usec / 1000000.0) );
    dSys  = ( ((double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1000000.0) -
              ((double)m_usage.ru_stime.tv_sec + (double)m_usage.ru_stime.tv_usec / 1000000.0) );
  }
#endif

  g_application.GetPerformanceStats().AddSample(m_statName, PerformanceCounter(elapsed,dUser,dSys));

  Reset();
}

double CPerformanceSample::GetEstimatedError()
{
  if (m_tmFreq.QuadPart == 0LL)
    QueryPerformanceFrequency(&m_tmFreq);

  LARGE_INTEGER tmStart, tmEnd;
  QueryPerformanceCounter(&tmStart);

  for (int i=0; i<100000;i++)
  {
    LARGE_INTEGER tmDummy;
    QueryPerformanceCounter(&tmDummy);
  }

  QueryPerformanceCounter(&tmEnd);
  double elapsed = (double)(tmEnd.QuadPart - tmStart.QuadPart) / (double)m_tmFreq.QuadPart;

  return (elapsed / 100000.0) * 2.0; // one measure at start time and another when done.
}



