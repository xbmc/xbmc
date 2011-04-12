// Event.h: interface for the CEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_)
#define AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_

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

#if defined(_WIN32)
#include <windows.h>
#else
#include "PlatformInclude.h"
#endif

class CEvent
{
public:
  bool WaitMSec(unsigned int milliSeconds);
  HANDLE GetHandle();
  void Reset();
  void Set();
  void Wait();
  CEvent(bool manual = false);
  CEvent(const CEvent& event);
  CEvent& operator=(const CEvent& src);

  virtual ~CEvent();

protected:
  HANDLE m_hEvent;
};

#endif // !defined(AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_)
