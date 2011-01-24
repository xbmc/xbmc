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

#include "system.h" // for HANDLE, CRITICALSECTION

class CSharedSection
{

public:
  CSharedSection();
  CSharedSection(const CSharedSection& src);
  CSharedSection& operator=(const CSharedSection& src);
  virtual ~CSharedSection();

  void EnterExclusive();
  void LeaveExclusive();

  void EnterShared();
  void LeaveShared();

private:

  CRITICAL_SECTION m_critSection;

  HANDLE m_eventFree;
  bool m_exclusive;
  long m_sharedLock;
};

class CSharedLock
{
public:
  CSharedLock(CSharedSection& cs);
  CSharedLock(const CSharedSection& cs);
  virtual ~CSharedLock();

  bool IsOwner() const;
  bool Enter();
  void Leave();

protected:
  CSharedLock(const CSharedLock& src);
  CSharedLock& operator=(const CSharedLock& src);

  // Reference to critical section object
  CSharedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

class CExclusiveLock
{
public:
  CExclusiveLock(CSharedSection& cs);
  CExclusiveLock(const CSharedSection& cs);
  virtual ~CExclusiveLock();

  bool IsOwner() const;
  bool Enter();
  void Leave();

protected:
  CExclusiveLock(const CExclusiveLock& src);
  CExclusiveLock& operator=(const CExclusiveLock& src);

  // Reference to critical section object
  CSharedSection& m_cs;
  // Ownership flag
  bool m_bIsOwner;
};

