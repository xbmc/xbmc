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

// AdminSocket.h: Schnittstelle für die Klasse CAdminSocket.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADMINSOCKET_H__C8A04733_3DF9_41C9_B596_DCDE8246AE88__INCLUDED_)
#define AFX_ADMINSOCKET_H__C8A04733_3DF9_41C9_B596_DCDE8246AE88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AsyncSocketEx.h"

class CAdminInterface;
class CAdminSocket : public CAsyncSocketEx  
{
public:
	BOOL CheckForTimeout();
	BOOL SendCommand(int nType, int nID, const void *pData, int nDataLength);
	BOOL Init();
	CAdminSocket(CAdminInterface *pAdminInterface);
	virtual ~CAdminSocket();

protected:
	BOOL FinishLogon();
	BOOL SendCommand(LPCTSTR pszCommand, int nTextType);
	CAdminInterface *m_pAdminInterface;
	BOOL ParseRecvBuffer();
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	struct t_data
	{
		unsigned char *pData;
		DWORD dwOffset;
		DWORD dwLength;
	};
	std::list<t_data> m_SendBuffer;

	unsigned char *m_pRecvBuffer;
	int m_nRecvBufferLen;
	DWORD m_nRecvBufferPos;

	BOOL m_bStillNeedAuth;
	unsigned char m_Nonce1[8];
	unsigned char m_Nonce2[8];
	FILETIME m_LastRecvTime;
};

#endif // !defined(AFX_ADMINSOCKET_H__C8A04733_3DF9_41C9_B596_DCDE8246AE88__INCLUDED_)
