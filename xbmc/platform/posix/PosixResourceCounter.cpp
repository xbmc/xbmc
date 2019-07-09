/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PosixResourceCounter.h"

#include "utils/log.h"

#include <errno.h>

#include "PlatformDefs.h"

CPosixResourceCounter::CPosixResourceCounter()
{
  Reset();
}

CPosixResourceCounter::~CPosixResourceCounter() = default;

double CPosixResourceCounter::GetCPUUsage()
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

void CPosixResourceCounter::Reset()
{
  if (gettimeofday(&m_tmLastCheck, NULL) == -1)
    CLog::Log(LOGERROR, "error %d in gettimeofday", errno);

  if (getrusage(RUSAGE_SELF, &m_usage) == -1)
    CLog::Log(LOGERROR, "error %d in getrusage", errno);

  m_dLastUsage = 0.0;
}





