
/*
 *      Copyright (C) 2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SemaphoreDarwin.h"
#include "utils/TimeUtils.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdio>

#define SEM_NAME_LEN (NAME_MAX-4)

CSemaphoreDarwin::CSemaphoreDarwin(uint32_t initialCount)
  : ISemaphore()
{
  m_szName            = new char[SEM_NAME_LEN];     

  snprintf(m_szName, SEM_NAME_LEN, "/xbmc-sem-%d-%" PRIu64,
      getpid(), CurrentHostCounter());

  m_pSem = sem_open(m_szName, O_CREAT, 0600, initialCount);
}

CSemaphoreDarwin::~CSemaphoreDarwin()
{
  sem_close(m_pSem);
  sem_unlink(m_szName);
  delete [] m_szName;
}

bool CSemaphoreDarwin::Wait()
{
  return (0 == sem_wait(m_pSem));
}

SEM_GRAB CSemaphoreDarwin::TimedWait(uint32_t millis)
{
  int64_t end = CurrentHostCounter() + (millis * 1000000LL);
  do {
    if (0 == sem_trywait(m_pSem))
      return SEM_GRAB_SUCCESS;
    if (errno != EAGAIN)
      return SEM_GRAB_FAILED;
    usleep(1000);
  } while (CurrentHostCounter() < end);

  return SEM_GRAB_TIMEOUT;
}

bool CSemaphoreDarwin::TryWait()
{
  return (0 == sem_trywait(m_pSem));
}

bool CSemaphoreDarwin::Post()
{
  return (0 == sem_post(m_pSem));
}

int CSemaphoreDarwin::GetCount() const
{
  int val;
  if (0 == sem_getvalue(m_pSem, &val))
    return val;
  return -1;
}

