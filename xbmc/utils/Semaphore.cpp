
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

#include "Semaphore.hpp"
#ifdef __linux__
#include "SemaphorePOSIX.h"
#elif defined(__APPLE__)
#include "SemaphoreDarwin.h"
#endif

CSemaphore::CSemaphore(uint32_t initialCount/*=1*/)
  : ISemaphore()
{
#ifdef _SEMAPHORE_H
  m_pSemaphore = new CSemaphorePOSIX(initialCount);
#elif defined(_BSD_SEMAPHORE_H)
  m_pSemaphore = new CSemaphoreDarwin(initialCount);
#else
#error No supported semaphore implementation available
#endif
}

CSemaphore::CSemaphore(const CSemaphore& sem)
  : ISemaphore()
{
#ifdef _SEMAPHORE_H
  m_pSemaphore = new CSemaphorePOSIX(sem.GetCount());
#elif defined(_BSD_SEMAPHORE_H)
  m_pSemaphore = new CSemaphoreDarwin(sem.GetCount());
#else
#error No supported semaphore implementation available
#endif
}

CSemaphore::~CSemaphore()
{
  delete m_pSemaphore;
}

bool CSemaphore::Wait()
{
  return m_pSemaphore->Wait();
}

SEM_GRAB CSemaphore::TimedWait(uint32_t millis)
{
  return m_pSemaphore->TimedWait(millis);
}

bool CSemaphore::TryWait()
{
  return m_pSemaphore->TryWait();
}

bool CSemaphore::Post()
{
  return m_pSemaphore->Post();
}

int CSemaphore::GetCount() const
{
  return m_pSemaphore->GetCount();
}

