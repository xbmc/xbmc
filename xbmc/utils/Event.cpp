
#include "../stdafx.h"
/*
 * XBoxMediaPlayer
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
#include "Event.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEvent::CEvent()
{
	m_hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
}



CEvent::~CEvent()
{
	CloseHandle(m_hEvent);
}


void CEvent::Wait()
{
	if (m_hEvent) 
	{
		WaitForSingleObject(m_hEvent,INFINITE);
	}
}

void CEvent::Set()
{
	if (m_hEvent) SetEvent(m_hEvent);
}

void CEvent::Reset()
{

	if (m_hEvent) ResetEvent(m_hEvent);
}

HANDLE CEvent::GetHandle()
{
	return m_hEvent;
}

bool CEvent::WaitMSec(DWORD dwMillSeconds)
{

	if (m_hEvent) 
	{
		DWORD dwResult=WaitForSingleObject(m_hEvent,dwMillSeconds);
		if (dwResult==WAIT_OBJECT_0) return true;
	}
	return false;
}

void CEvent::PulseEvent()
{
	::PulseEvent(m_hEvent);
}
