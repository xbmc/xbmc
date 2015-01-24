//////////////////////////////////////////////////////////////////////
//
// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CRITICAL_SECTION_H_
#define _CRITICAL_SECTION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#ifdef _LINUX
#include "PlatformDefs.h"
#include "linux/XSyncUtils.h"
#include "XCriticalSection.h"
#else
#include "win32/XCriticalSection.h"
#endif

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

class CCriticalSection
{
public:
  // Constructor/destructor.
  CCriticalSection();
  virtual ~CCriticalSection();

  XCriticalSection& getCriticalSection() { return m_criticalSection; }

private:
  XCriticalSection m_criticalSection;

  //don't allow copying a CCriticalSection
  CCriticalSection(const CCriticalSection& section) {}
  CCriticalSection& operator=(const CCriticalSection& section) {return *this;}
};

// The CCritical section overloads.
void InitializeCriticalSection(CCriticalSection* section);
void DeleteCriticalSection(CCriticalSection* section);
BOOL OwningCriticalSection(CCriticalSection* section);
DWORD ExitCriticalSection(CCriticalSection* section);
void RestoreCriticalSection(CCriticalSection* section, DWORD count);
void EnterCriticalSection(CCriticalSection* section);
void LeaveCriticalSection(CCriticalSection* section);

// And a few special ones.
void EnterCriticalSection(CCriticalSection& section);
void LeaveCriticalSection(CCriticalSection& section);
BOOL OwningCriticalSection(CCriticalSection& section);
DWORD ExitCriticalSection(CCriticalSection& section);
void RestoreCriticalSection(CCriticalSection& section, DWORD count);

#endif
