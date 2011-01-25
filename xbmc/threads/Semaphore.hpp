
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

#ifndef SEMAPHORE_H__
#define SEMAPHORE_H__

#include "ISemaphore.h"

class CSemaphore : public ISemaphore
{
    ISemaphore* m_pSemaphore;
  public:
                      CSemaphore(uint32_t initialCount=1);
                      CSemaphore(const CSemaphore& sem);
    virtual           ~CSemaphore();
    virtual bool      Wait();
    virtual SEM_GRAB  TimedWait(uint32_t millis);
    virtual bool      TryWait();
    virtual bool      Post();
    virtual int       GetCount() const;
};

#endif // SEMAPHORE_H__

