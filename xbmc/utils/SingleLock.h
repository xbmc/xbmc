// SingleLock.h: interface for the CSingleLock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_)
#define AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
 * XBMC Media Center
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "CriticalSection.h"
class CSingleLock
{
public:
  void Unlock();
  void Lock();

  CSingleLock(CCriticalSection& cs);
  CSingleLock(const CCriticalSection& cs);
  virtual ~CSingleLock();

  bool IsOwner() const;
  bool Enter();
  void Leave();

private:
  CSingleLock(const CSingleLock& src);
  CSingleLock& operator=(const CSingleLock& src);

  // Reference to critical section object
  CCriticalSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

class CSingleExit
{
public:
  CSingleExit(CCriticalSection& cs);
  CSingleExit(const CCriticalSection& cs);
  virtual ~CSingleExit();

  void Exit();
  void Restore();

  CCriticalSection& m_cs;
  unsigned int      m_count;
};

#endif // !defined(AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_)
