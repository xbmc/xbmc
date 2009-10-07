#pragma once

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

//  CMutex - Wrapper for xbox Mutex API
//
// by Bobbin007 in 2003

#include "system.h" // for HANDLE

class CMutex
{
public:
  CMutex();
  CMutex( char* pName );
  virtual ~CMutex();

  HANDLE GetHandle();

  void Release();

  bool Wait();
  bool WaitMSec(unsigned int milliSeconds);

protected:
  HANDLE m_hMutex;
};

class CMutexWait
{
public:
  CMutexWait(CMutex& mutex);
  virtual ~CMutexWait();
private:
  CMutex& m_mutex;
  bool m_bLocked;
};
