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

#if !defined(AFX_EXTERNALIPCHECK_H__B077D8D7_6883_4DD9_9B0A_2C99562D0930__INCLUDED_)
#define AFX_EXTERNALIPCHECK_H__B077D8D7_6883_4DD9_9B0A_2C99562D0930__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AsyncSocketEx.h"

class CServerThread;

class CExternalIpCheck : public CAsyncSocketEx  
{
public:
	virtual void OnReceive(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	CExternalIpCheck(CServerThread *pOwner);
	virtual ~CExternalIpCheck();
	virtual void OnTimer();
	virtual void TriggerUpdate();

protected:
	virtual void Start();

// Variables
public:
	virtual CStdString GetIP() const;
	int m_nTimerID;
	

protected:
	CStdString m_IP;
	int m_nFailedConnections;
	int m_nRetryCount;
	int m_nElapsedSeconds;
	CServerThread *m_pOwner;
	BOOL m_bActive;
	BOOL m_bTriggerUpdateCalled;
};

#endif // !defined(AFX_EXTERNALIPCHECK_H__B077D8D7_6883_4DD9_9B0A_2C99562D0930__INCLUDED_)
