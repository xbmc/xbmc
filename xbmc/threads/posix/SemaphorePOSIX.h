
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

#ifndef SEMAPHORE_POSIX_H__
#define SEMAPHORE_POSIX_H__

#include "threads/ISemaphore.h"
#include <semaphore.h>

class CSemaphorePOSIX : public ISemaphore
{
    char*   m_szName;
    sem_t*  m_pSem;
  public:
                      CSemaphorePOSIX(uint32_t initialCount);
    virtual           ~CSemaphorePOSIX();
    virtual bool      Wait();
    virtual SEM_GRAB  TimedWait(uint32_t millis);
    virtual bool      TryWait();
    virtual bool      Post();
    virtual int       GetCount() const;
};

#endif // SEMAPHORE_POSIX_H__

