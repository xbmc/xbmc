
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

#ifndef ISEMAPHORE_H__
#define ISEMAPHORE_H__

#include <stdint.h>

enum SEM_GRAB
{
  SEM_GRAB_SUCCESS,
  SEM_GRAB_FAILED,
  SEM_GRAB_TIMEOUT
};

class ISemaphore
{
  public:
                      ISemaphore()  {}
    virtual           ~ISemaphore() {}
    virtual bool      Wait()                      = 0;
    virtual SEM_GRAB  TimedWait(uint32_t millis)  = 0;
    virtual bool      TryWait()                   = 0;
    virtual bool      Post()                      = 0;
    virtual int       GetCount() const            = 0;
};

#endif // ISEMAPHORE_H__

