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

// SpeedLimit.h: interface for the CSpeedLimit class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPEEDLIMIT_H__D4CEBB35_CE08_4438_9A42_4F565DE84AE4__INCLUDED_)
#define AFX_SPEEDLIMIT_H__D4CEBB35_CE08_4438_9A42_4F565DE84AE4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSpeedLimit
{
public:
	bool IsItActive(const SYSTEMTIME &time) const;
	CSpeedLimit();
	virtual ~CSpeedLimit();

	virtual int GetRequiredBufferLen() const;
	virtual char * FillBuffer(char *p) const;
	virtual unsigned char * ParseBuffer(unsigned char *pBuffer, int length);

	BOOL		m_DateCheck;
	struct t_date
	{
		int y, m, d;
	} m_Date;

	BOOL		m_FromCheck;
	BOOL		m_ToCheck;
	struct t_time
	{
		int h, m, s;
	} m_FromTime, m_ToTime;
	int			m_Speed;
	int m_Day;
};

typedef std::vector<CSpeedLimit> SPEEDLIMITSLIST;

#endif // !defined(AFX_SPEEDLIMIT_H__D4CEBB35_CE08_4438_9A42_4F565DE84AE4__INCLUDED_)
