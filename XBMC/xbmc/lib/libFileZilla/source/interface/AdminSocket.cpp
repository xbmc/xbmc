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

// AdminSocket.cpp: Implementierung der Klasse CAdminSocket.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AdminSocket.h"
#include "MainFrm.h"
#include "..\OptionTypes.h"
#include "misc\md5.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

#define BUFSIZE 4096

CAdminSocket::CAdminSocket(CMainFrame *pMainFrame)
{
	ASSERT(pMainFrame);
	m_pMainFrame = pMainFrame;
	m_pRecvBuffer = new unsigned char[BUFSIZE];
	m_nRecvBufferLen = BUFSIZE;
	m_nRecvBufferPos = 0;
	m_nConnectionState = 0;
	m_bClosed = FALSE;
}

CAdminSocket::~CAdminSocket()
{
	delete [] m_pRecvBuffer;
	
	for (std::list<t_data>::iterator iter=m_SendBuffer.begin(); iter!=m_SendBuffer.end(); iter++)
		delete [] iter->pData;
}

void CAdminSocket::OnConnect(int nErrorCode)
{
	if (!nErrorCode)
	{
		if (!m_nConnectionState)
		{
			m_pMainFrame->ShowStatus(_T("Connected, waiting for authentication"), 0);
			m_nConnectionState = 1;
		}
	}
	else
	{
		m_pMainFrame->ShowStatus(_T("Error, could not connect to server"), 1);
		Close();
	}
}

void CAdminSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode)
	{
		m_pMainFrame->ShowStatus(_T("OnReceive failed, closing connection"), 1);
		Close();
		return;
	}
	
	if (!m_nConnectionState)
	{
		m_pMainFrame->ShowStatus(_T("Connected, waiting for authentication"), 0);
		m_nConnectionState = 1;
	}

	int numread=Receive(m_pRecvBuffer + m_nRecvBufferPos, m_nRecvBufferLen - m_nRecvBufferPos);
	while (numread > 0)
	{
		m_nRecvBufferPos += numread;
		if ( (m_nRecvBufferLen-m_nRecvBufferPos) < (BUFSIZE/4))
		{
			unsigned char *tmp=m_pRecvBuffer;
			m_pRecvBuffer = new unsigned char[m_nRecvBufferLen + BUFSIZE];
			memcpy(m_pRecvBuffer, tmp, m_nRecvBufferPos);
			m_nRecvBufferLen += BUFSIZE;
			delete [] tmp;
		}
		numread=Receive(m_pRecvBuffer + m_nRecvBufferPos, m_nRecvBufferLen - m_nRecvBufferPos);
	}
	if (numread == 0)
	{
		ParseRecvBuffer();
		Close();
		return;
	}
	else if (numread == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			ParseRecvBuffer();
			Close();
			return;
		}
	}
	while (ParseRecvBuffer());
}

void CAdminSocket::OnSend(int nErrorCode)
{
	if (nErrorCode)
	{
		Close();
		return;
	}
	if (!m_nConnectionState)
		return;

	while (!m_SendBuffer.empty())
	{
		t_data data=m_SendBuffer.front();
		int nSent = Send(data.pData+data.dwOffset, data.dwLength-data.dwOffset);
		if (!nSent)
		{
			Close();
			return;
		}
		if (nSent == SOCKET_ERROR)
		{
			if (WSAGetLastError()!=WSAEWOULDBLOCK)
				Close();
			return;
		}
		
		if (nSent<(data.dwLength-data.dwOffset))
			data.dwOffset+=nSent;
		else
		{
			m_SendBuffer.pop_front();
			delete [] data.pData;
		}
	}
}

void CAdminSocket::Close()
{
	if (m_nConnectionState)
		m_pMainFrame->ShowStatus(_T("Connection to server closed."), 1);
	m_nConnectionState = 0;
	if (!m_bClosed)
	{
		m_bClosed = TRUE;
		m_pMainFrame->CloseAdminSocket();
	}
}

BOOL CAdminSocket::ParseRecvBuffer()
{
	DWORD len;
	switch (m_nConnectionState)
	{
	case 1:
		{
			if (m_nRecvBufferPos<3)
				return FALSE;
			if (m_pRecvBuffer[0]!='F' || m_pRecvBuffer[1]!='Z' || m_pRecvBuffer[2]!='S')
			{
				CString str;
				str.Format(_T("Protocol error: Unknown protocol identifier (0x%d 0x%d 0x%d)."), (int)m_pRecvBuffer[0], (int)m_pRecvBuffer[1], (int)m_pRecvBuffer[2]);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}
			if (m_nRecvBufferPos<5)
				return FALSE;
			int len = m_pRecvBuffer[3]*256 + m_pRecvBuffer[4];
			if (len != 4)
			{
				CString str;
				str.Format(_T("Protocol error: Invalid server version length (%d)."), len);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}
			if (m_nRecvBufferPos<9)
				return FALSE;
			int version;
			memcpy(&version, m_pRecvBuffer + 5, 4);
			if (version != SERVER_VERSION)
			{
				CString str;
				str.Format(_T("Protocol warning: Server version mismatch: Server version is %d.%d.%d.%d, interface version is %d.%d.%d.%d"),
						   (version >> 24) & 0xFF,
						   (version >> 16) & 0xFF,
						   (version >>  8) & 0xFF,
						   (version >>  0) & 0xFF,
						   (SERVER_VERSION >> 24) & 0xFF,
						   (SERVER_VERSION >> 16) & 0xFF,
						   (SERVER_VERSION >>  8) & 0xFF,
						   (SERVER_VERSION >>  0) & 0xFF);
				m_pMainFrame->ShowStatus(str, 1);
			}

			if (m_nRecvBufferPos<11)
				return FALSE;
			len = m_pRecvBuffer[9]*256 + m_pRecvBuffer[10];
			if (len != 4)
			{
				CString str;
				str.Format(_T("Protocol error: Invalid protocol version length (%d)."), len);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}
			if (m_nRecvBufferPos<15)
				return FALSE;
			memcpy(&version, m_pRecvBuffer + 11, 4);
			if (version != PROTOCOL_VERSION)
			{
				CString str;
				str.Format(_T("Protocol error: Protocol version mismatch: Server protocol version is %d.%d.%d.%d, interface protocol version is %d.%d.%d.%d"),
						   (version >> 24) & 0xFF,
						   (version >> 16) & 0xFF,
						   (version >>  8) & 0xFF,
						   (version >>  0) & 0xFF,
						   (PROTOCOL_VERSION >> 24) & 0xFF,
						   (PROTOCOL_VERSION >> 16) & 0xFF,
						   (PROTOCOL_VERSION >>  8) & 0xFF,
						   (PROTOCOL_VERSION >>  0) & 0xFF);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}

			memmove(m_pRecvBuffer, m_pRecvBuffer+15, m_nRecvBufferPos-15);
			m_nRecvBufferPos-=15;
			m_nConnectionState = 2;
		}
		break;
	case 2:
		if (m_nRecvBufferPos<5)
			return FALSE;
		if ((m_pRecvBuffer[0]&0x03) > 2)
		{
			CString str;
			str.Format(_T("Protocol error: Unknown command type (%d), closing connection."), (int)(m_pRecvBuffer[0]&0x03));
			m_pMainFrame->ShowStatus(str, 1);
			Close();
			return FALSE;
		}
		memcpy(&len, m_pRecvBuffer+1, 4);
		if (len + 5 <= m_nRecvBufferPos)
		{
			if ((m_pRecvBuffer[0]&0x03) == 0  &&  (m_pRecvBuffer[0]&0x7C)>>2 == 0)
			{
				if (len<4)
				{
					m_pMainFrame->ShowStatus(_T("Invalid auth data"), 1);
					Close();
					return FALSE;
				}
				unsigned char *p = m_pRecvBuffer + 5;
				
				int noncelen1 = *p*256 + p[1];
				if ((noncelen1+2) > (len-2))
				{
					m_pMainFrame->ShowStatus(_T("Invalid auth data"), 1);
					Close();
					return FALSE;
				}
				
				int noncelen2 = p[2 + noncelen1]*256 + p[2 + noncelen1 +1];
				if ((noncelen1+noncelen2+4) > len)
				{
					m_pMainFrame->ShowStatus(_T("Invalid auth data"), 1);
					Close();
					return FALSE;
				}
				
				MD5 md5;
				if (noncelen1)
					md5.update(p+2, noncelen1);
				md5.update((const unsigned char *)(const char *)m_Password, m_Password.GetLength());
				if (noncelen2)
					md5.update(p+noncelen1+4, noncelen2);
				md5.finalize();
				
				memmove(m_pRecvBuffer, m_pRecvBuffer+len+5, m_nRecvBufferPos-len-5);
				m_nRecvBufferPos-=len+5;
				
				unsigned char *digest = md5.raw_digest();
				SendCommand(0, digest, 16);
				delete [] digest;
				m_nConnectionState=3;
				return TRUE;
			}
			else if ((m_pRecvBuffer[0]&0x03) == 1  &&  (m_pRecvBuffer[0]&0x7C)>>2 == 0)
			{
				m_nConnectionState=3;
				m_pMainFrame->ParseReply((m_pRecvBuffer[0]&0x7C)>>2, m_pRecvBuffer+5, len);
			}
			else
			{
				CString str;
				str.Format(_T("Protocol error: Unknown command ID (%d), closing connection."), (int)(m_pRecvBuffer[0]&0x7C)>>2);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}
			memmove(m_pRecvBuffer, m_pRecvBuffer+len+5, m_nRecvBufferPos-len-5);
			m_nRecvBufferPos-=len+5;
		}
		break;
	case 3:
		if (m_nRecvBufferPos<5)
			return FALSE;
		int nType = *m_pRecvBuffer & 0x03;
		int nID = (*m_pRecvBuffer & 0x7C) >> 2;
		if (nType>2 || nType<1)
		{
			CString str;
			str.Format(_T("Protocol error: Unknown command type (%d), closing connection."), nType);
			m_pMainFrame->ShowStatus(str, 1);
			Close();
			return FALSE;
		}
		else
		{
			memcpy(&len, m_pRecvBuffer+1, 4);
			if (len > 0xFFFFFF)
			{
				CString str;
				str.Format(_T("Protocol error: Invalid data length (%u) for command (%d:%d)"), len, nType, nID);
				m_pMainFrame->ShowStatus(str, 1);
				Close();
				return FALSE;
			}				
			if (m_nRecvBufferPos < len+5)
				return FALSE;
			else
			{
				unsigned char *pData = new unsigned char[len];
				memcpy(pData, m_pRecvBuffer+5, len);
				memmove(m_pRecvBuffer, m_pRecvBuffer+len+5, m_nRecvBufferPos-len-5);
				m_nRecvBufferPos-=len+5;

				if (nType==1)
					m_pMainFrame->ParseReply(nID, pData, len);
				else if (nType==2)
					m_pMainFrame->ParseStatus(nID, pData, len);
				else
				{
					CString str;
					str.Format(_T("Protocol warning: Command type %d not implemented."), nType);
					m_pMainFrame->ShowStatus(str, 1);
				}
				delete [] pData;
			}
		}
		break;
	}
	return TRUE;
}

BOOL CAdminSocket::SendCommand(int nType)
{
	t_data data;
	data.pData = new unsigned char[5];
	data.pData[0] = nType << 2;
	data.dwOffset = 0;
	DWORD dwDataLength = 0;
	memcpy(data.pData + 1, &dwDataLength, 4);

	data.dwLength = 5;

	m_SendBuffer.push_back(data);
	
	do 
	{
		data=m_SendBuffer.front();
		int nSent = Send(data.pData+data.dwOffset, data.dwLength-data.dwOffset);
		if (!nSent)
		{
			Close();
			return FALSE;
		}
		if (nSent == SOCKET_ERROR)
		{
			if (WSAGetLastError()!=WSAEWOULDBLOCK)
			{
				Close();
				return FALSE;
			}
			return TRUE;
		}
		
		if (nSent<(data.dwLength-data.dwOffset))
			data.dwOffset+=nSent;
		else
		{
			m_SendBuffer.pop_front();
			delete [] data.pData;
		}
	} while (!m_SendBuffer.empty());

	return TRUE;
}

BOOL CAdminSocket::SendCommand(int nType, void *pData, int nDataLength)
{
	ASSERT((pData && nDataLength) || (!pData && !nDataLength));
	
	t_data data;
	data.pData = new unsigned char[nDataLength+5];
	data.pData[0] = nType << 2;
	data.dwOffset = 0;
	memcpy(data.pData + 1, &nDataLength, 4);
	if (pData)
		memcpy(data.pData+5, pData, nDataLength);

	data.dwLength = nDataLength + 5;

	m_SendBuffer.push_back(data);
	
	do 
	{
		data=m_SendBuffer.front();
		int nSent = Send(data.pData+data.dwOffset, data.dwLength-data.dwOffset);
		if (!nSent)
		{
			Close();
			return FALSE;
		}
		if (nSent == SOCKET_ERROR)
		{
			if (WSAGetLastError()!=WSAEWOULDBLOCK)
			{
				Close();
				return FALSE;
			}
			return TRUE;
		}
		
		if (nSent<(data.dwLength-data.dwOffset))
			data.dwOffset+=nSent;
		else
		{
			m_SendBuffer.pop_front();
			delete [] data.pData;
		}
	} while (!m_SendBuffer.empty());

	return TRUE;
}

BOOL CAdminSocket::IsConnected()
{
	return m_nConnectionState == 3;
}

void CAdminSocket::OnClose(int nErrorCode)
{
	Close();
}