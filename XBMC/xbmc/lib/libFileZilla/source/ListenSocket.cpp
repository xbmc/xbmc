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

// ListenSocket.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "ListenSocket.h"
#include "ServerThread.h"
#include "Server.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListenSocket

CListenSocket::CListenSocket(CServer *pServer)
{
	ASSERT(pServer);
	m_pServer = pServer;

	m_bLocked = FALSE;
}

CListenSocket::~CListenSocket()
{
}

/////////////////////////////////////////////////////////////////////////////
// Member-Funktion CListenSocket 

void CListenSocket::OnAccept(int nErrorCode) 
{
	CAsyncSocketEx socket;
	if (!Accept(socket))
	{
		int nError = WSAGetLastError();
		CStdString str;
		str.Format("Failure in CListenSocket::OnAccept(%d) - call to CAsyncSocketEx::Accept failed, errorcode %d", nErrorCode, nError);
		SendStatus(str, 1);
		SendStatus("If you use a firewall, please check your firewall configuration", 1);
		return;
	}

	if (m_bLocked)
	{
		CStdString str = "421 Server is locked, please try again later.\r\n";
		socket.Send(str, str.GetLength());
		return;
	}
	
	int minnum = 255*255*255;
	CServerThread *pBestThread=0;;
	for (std::list<CServerThread *>::iterator iter=m_pThreadList->begin(); iter!=m_pThreadList->end(); iter++)
	{
		int num=(*iter)->GetNumConnections();
		if (num<minnum && (*iter)->IsReady())
		{
			minnum=num;
			pBestThread=*iter;
			if (!num)
				break;
		}
	}
	if (!pBestThread)
	{
		char str[] = "421 Server offline.";
		socket.Send(str, strlen(str)+1);
		socket.Close();
		return;
	}
	
	/* Disable Nagle algorithm. Most of the time single short strings get 
	 * transferred over the control connection. Waiting for additional data
	 * where there will be most likely none affects performance.
	 */
	BOOL value = TRUE;
	socket.SetSockOpt(TCP_NODELAY, &value, sizeof(value), IPPROTO_TCP);

	SOCKET sockethandle = socket.Detach();
	
	pBestThread->AddSocket(sockethandle);
		
	CAsyncSocketEx::OnAccept(nErrorCode);
}

void CListenSocket::SendStatus(CStdString status, int type)
{
	m_pServer->ShowStatus(status, type);
}