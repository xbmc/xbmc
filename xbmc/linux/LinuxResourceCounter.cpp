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

#include "PlatformInclude.h"
#include "LinuxResourceCounter.h"
#include "utils/log.h"

#include <errno.h>

CLinuxResourceCounter::CLinuxResourceCounter()
{
  Reset();
}

CLinuxResourceCounter::~CLinuxResourceCounter()
{
}

double CLinuxResourceCounter::GetCPUUsage()
{
  struct timeval tmNow;
  if (gettimeofday(&tmNow, NULL) == -1)
    CLog::Log(LOGERROR, "error %d in gettimeofday", errno);
  else
  {
    double dElapsed = ( ((double)tmNow.tv_sec + (double)tmNow.tv_usec / 1000000.0) -
                 ((double)m_tmLastCheck.tv_sec + (double)m_tmLastCheck.tv_usec / 1000000.0) );

    if (dElapsed >= 3.0)
    {
      struct rusage usage;
      if (getrusage(RUSAGE_SELF, &usage) == -1)
        CLog::Log(LOGERROR,"error %d in getrusage", errno);
      else
      {
        double dUser = ( ((double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0) -
                         ((double)m_usage.ru_utime.tv_sec + (double)m_usage.ru_utime.tv_usec / 1000000.0) );
        double dSys  = ( ((double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1000000.0) -
                  ((double)m_usage.ru_stime.tv_sec + (double)m_usage.ru_stime.tv_usec / 1000000.0) );

        m_tmLastCheck = tmNow;
        m_usage = usage;
        m_dLastUsage = ((dUser+dSys) / dElapsed) * 100.0;
        return m_dLastUsage;
      }
    }
  }

  return m_dLastUsage;
}

void CLinuxResourceCounter::Reset()
{
  if (gettimeofday(&m_tmLastCheck, NULL) == -1)
    CLog::Log(LOGERROR, "error %d in gettimeofday", errno);

  if (getrusage(RUSAGE_SELF, &m_usage) == -1)
    CLog::Log(LOGERROR, "error %d in getrusage", errno);

  m_dLastUsage = 0.0;
}





