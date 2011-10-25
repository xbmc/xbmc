/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CriticalSection.h"

CCriticalSection::CCriticalSection(void)
{
#ifdef _WIN32
  InitializeCriticalSection(&m_CriticalSection);
#else
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE_NP)
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&m_CriticalSection, &attr);
  pthread_mutexattr_destroy(&attr);
#endif
}

CCriticalSection::~CCriticalSection(void)
{
#ifdef _WIN32
  DeleteCriticalSection(&m_CriticalSection);
#else
  pthread_mutex_destroy(&m_CriticalSection);
#endif
}

void CCriticalSection::Lock(void)
{
#ifdef _WIN32
  EnterCriticalSection(&m_CriticalSection);
#else
  (void)pthread_mutex_lock(&m_CriticalSection);
#endif
}

void CCriticalSection::Unlock(void)
{
#ifdef _WIN32
  LeaveCriticalSection(&m_CriticalSection);
#else
  pthread_mutex_unlock(&m_CriticalSection);
#endif
}
