#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "CriticalSection.h"
#ifdef _LINUX
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#endif

class cRwLock
{
public:
  cRwLock(bool PreferWriter = false);
  virtual ~cRwLock(void);

  bool ReadLock(int TimeoutMs = 0);
  void ReadUnlock();
  bool WriteLock(int TimeoutMs = 0);
  void WriteUnlock();

private:
#ifdef _LINUX
  pthread_rwlock_t m_rwLock;
#else
  HANDLE m_hWriterEvent;
  HANDLE m_hNoReadersEvent;
  int m_nReaderCount;
#endif
};
