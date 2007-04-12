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

#if !defined(AFX_ADMINSOCKET_H__4FDF0C68_9EA7_440B_A4ED_2DC358E4A054__INCLUDED_)
#define AFX_ADMINSOCKET_H__4FDF0C68_9EA7_440B_A4ED_2DC358E4A054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AsyncSocketEx.h"

class CMainFrame;
class CAdminSocket : public CAsyncSocketEx  
{
public:
	CString m_Password;
	BOOL IsConnected();
	BOOL SendCommand(int nType);
	BOOL SendCommand(int nType, void *pData, int nDataLength);
	BOOL ParseRecvBuffer();
	virtual void Close();
	CAdminSocket(CMainFrame *pMainFrame);
	virtual ~CAdminSocket();

protected:
	struct t_data
	{
		unsigned char *pData;
		DWORD dwOffset;
		DWORD dwLength;
	};
	std::list<t_data> m_SendBuffer;

	BOOL m_bClosed;
	virtual void OnReceive(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	CMainFrame *m_pMainFrame;

	unsigned char *m_pRecvBuffer;
	int m_nRecvBufferLen;
	DWORD m_nRecvBufferPos;

	int m_nConnectionState;
};

#endif // !defined(AFX_ADMINSOCKET_H__4FDF0C68_9EA7_440B_A4ED_2DC358E4A054__INCLUDED_)
