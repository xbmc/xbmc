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

//////////////////////////////////////////////////////////////////////
CCriticalSection::CCriticalSection()
{
  m_criticalSection.Initialize();
}

//////////////////////////////////////////////////////////////////////
CCriticalSection::~CCriticalSection()
{
  m_criticalSection.Destroy();
}

// The C API.
void  InitializeCriticalSection(CCriticalSection* section)             { section->getCriticalSection().Initialize(); }
void  DeleteCriticalSection(CCriticalSection* section)                 { section->getCriticalSection().Destroy(); }
BOOL  OwningCriticalSection(CCriticalSection* section)                 { return section->getCriticalSection().Owning(); }
DWORD ExitCriticalSection(CCriticalSection* section)                   { return section->getCriticalSection().Exit(); }
void  RestoreCriticalSection(CCriticalSection* section, DWORD count)   { return section->getCriticalSection().Restore(count); }
void  EnterCriticalSection(CCriticalSection* section)                  { section->getCriticalSection().Enter(); }
void  LeaveCriticalSection(CCriticalSection* section)                  { section->getCriticalSection().Leave(); }

void EnterCriticalSection(CCriticalSection& section)                   { section.getCriticalSection().Enter(); }
void LeaveCriticalSection(CCriticalSection& section)                   { section.getCriticalSection().Leave(); }
BOOL OwningCriticalSection(CCriticalSection& section)                  { return section.getCriticalSection().Owning(); }
DWORD ExitCriticalSection(CCriticalSection& section)                   { return section.getCriticalSection().Exit(); }
void RestoreCriticalSection(CCriticalSection& section, DWORD count)    { return section.getCriticalSection().Restore(count); }

