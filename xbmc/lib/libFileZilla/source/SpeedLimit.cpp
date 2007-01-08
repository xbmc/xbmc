// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// SpeedLimit.cpp: implementation of the CSpeedLimit class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SpeedLimit.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSpeedLimit::CSpeedLimit()
{
	m_Day = 0;

	m_Speed = 10;
	m_ToCheck = FALSE;
	m_DateCheck = FALSE;
	m_FromCheck = FALSE;
}

CSpeedLimit::~CSpeedLimit()
{

}

bool CSpeedLimit::IsItActive(const SYSTEMTIME &time) const
{
	if (m_DateCheck)
	{
		if ((m_Date.y != time.wYear) ||
			(m_Date.m != time.wMonth) ||
			(m_Date.d != time.wDay))
			return false;
	}
	else
	{
		int i = (time.wDayOfWeek + 6) % 7;
		
		if (!(m_Day & ( 1 << i)))
			return false;
	}

	if (m_ToCheck)
	{
		if (m_ToTime.h < time.wHour)
			return false;

		if (m_ToTime.h == time.wHour)
		{
			if (m_ToTime.m < time.wMinute)
				return false;

			if (m_ToTime.m == time.wMinute)
			{
				if (m_ToTime.s < time.wSecond)
					return false;
			}
		}
	}

	if (m_FromCheck)
	{
		if (m_FromTime.h > time.wHour)
			return false;

		if (m_FromTime.h == time.wHour)
		{
			if (m_FromTime.m > time.wMinute)
				return false;

			if (m_FromTime.m == time.wMinute)
			{
				if (m_FromTime.s > time.wSecond)
					return false;
			}
		}
	}

	return true;
}

int CSpeedLimit::GetRequiredBufferLen() const
{
	return	2 + //Speed
		    4 + //date
			6 +	//2 * time
			1;  //Weekday
	
}

char * CSpeedLimit::FillBuffer(char *p) const
{
	*p++ = m_Speed >> 8;
	*p++ = m_Speed % 256;

	if (m_DateCheck)
	{
		*p++ = m_Date.y >> 8;
		*p++ = m_Date.y % 256;
		*p++ = m_Date.m;
		*p++ = m_Date.d;
	}
	else
	{
		memset(p, 0, 4);
		p += 4;
	}
	
	if (m_FromCheck)
	{
		*p++ = m_FromTime.h;
		*p++ = m_FromTime.m;
		*p++ = m_FromTime.s;
	}
	else
	{
		memset(p, 0, 3);
		p += 3;
	}

	if (m_ToCheck)
	{
		*p++ = m_ToTime.h;
		*p++ = m_ToTime.m;
		*p++ = m_ToTime.s;
	}
	else
	{
		memset(p, 0, 3);
		p += 3;
	}

	*p++ = m_Day;
	
	return p;
}

unsigned char * CSpeedLimit::ParseBuffer(unsigned char *pBuffer, int length)
{
	if (length < GetRequiredBufferLen())
		return NULL;

	unsigned char *p = pBuffer;

	m_Speed = *p++ << 8;
	m_Speed |= *p++;

	char tmp[4] = {0};

	if (memcmp(p, tmp, 4))	
	{
		m_DateCheck = TRUE;
		m_Date.y = *p++ << 8;
		m_Date.y |= *p++;
		m_Date.m = *p++;
		m_Date.d = *p++;
		if (m_Date.y < 1900 || m_Date.y > 3000 || m_Date.m < 1 || m_Date.m > 12 || m_Date.d < 1 || m_Date.d > 31)
			return FALSE;
	}
	else
	{
		p += 4;
		m_DateCheck = FALSE;
	}
	
	if (memcmp(p, tmp, 3))	
	{
		m_FromCheck = TRUE;
		m_FromTime.h = *p++;
		m_FromTime.m = *p++;
		m_FromTime.s = *p++;
		if (m_FromTime.h > 23 || m_FromTime.m > 59 || m_FromTime.s > 59)
			return FALSE;
	}
	else
	{
		p += 3;
		m_FromCheck = FALSE;
	}

	if (memcmp(p, tmp, 3))	
	{
		m_ToCheck = TRUE;
		m_ToTime.h = *p++;
		m_ToTime.m = *p++;
		m_ToTime.s = *p++;
		if (m_ToTime.h > 23 || m_ToTime.m > 59 || m_ToTime.s > 59)
			return FALSE;
	}
	else
	{
		p += 3;
		m_ToCheck = FALSE;
	}

	m_Day = *p++;

	return p;
}