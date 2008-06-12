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

// TransferSocket.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "TransferSocket.h"
#include "ControlSocket.h"
#include "options.h"
#if defined(_XBOX)
#include "util.h"
#include "GUISettings.h"
#endif
#include "ServerThread.h"
#ifndef NOLAYERS
#include "AsyncGssSocketLayer.h"
#endif
#include "Permissions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning (disable:4244)
#pragma warning (disable:4800)
/////////////////////////////////////////////////////////////////////////////
// CTransferSocket
CTransferSocket::CTransferSocket(CControlSocket *pOwner)
{
	ASSERT(pOwner);
	m_pOwner = pOwner;
	m_status = 0;
	m_nMode = TRANSFERMODE_NOTSET;

	m_nBufferPos = NULL;
	m_pBuffer = NULL;
	m_pDirListing = NULL;
	bAccepted = FALSE;

	m_bSentClose = FALSE;

	m_bReady = FALSE;
	m_bStarted = FALSE;
	GetSystemTime(&m_LastActiveTime);
	m_nRest = 0;

#ifndef NOLAYERS
	m_pGssLayer = NULL;
#endif

	m_hFile = INVALID_HANDLE_VALUE;

	m_nBufSize = m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_BUFFERSIZE);
}

void CTransferSocket::Init(t_dirlisting *pDir, int nMode)
{
	ASSERT(nMode==TRANSFERMODE_LIST || nMode==TRANSFERMODE_NLST);
	ASSERT(pDir);
	m_bReady = TRUE;
	m_status = 0;
	if (m_pBuffer)
#if defined(_XBOX)
		free(m_pBuffer);
#else
		delete [] m_pBuffer;
#endif
	m_pBuffer = 0;
	m_pDirListing = pDir;

	m_nMode = nMode;

	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	m_nBufferPos = 0;
}

void CTransferSocket::Init(CStdString filename, int nMode, _int64 rest, BOOL bBinary /*=TRUE*/)
{
	ASSERT(nMode==TRANSFERMODE_SEND || nMode==TRANSFERMODE_RECEIVE);
	m_bReady=TRUE;
	m_Filename=filename;
	m_nRest=rest;
	m_nMode=nMode;

#if defined(_XBOX)
  SXFTransferInfo* info = new SXFTransferInfo;
  info->mConnectionId = m_pOwner->m_userid;
  info->mFilename = m_Filename;
  info->mIsResumed = rest;
  info->mStatus = (nMode == TRANSFERMODE_SEND ? SXFTransferInfo::sendBegin : SXFTransferInfo::recvBegin);
  if (!PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_FILETRANSFER, (LPARAM)info))
		delete info;
#endif

}

CTransferSocket::~CTransferSocket()
{
	if (m_pBuffer)
#if defined(_XBOX)
		free(m_pBuffer);
#else
		delete [] m_pBuffer;
#endif
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
#ifndef NOLAYERS
	RemoveAllLayers();
	if (m_pGssLayer)
		delete m_pGssLayer;
#endif
	while (m_pDirListing)
	{
		t_dirlisting *pPrev = m_pDirListing;
		m_pDirListing = m_pDirListing->pNext;
		delete pPrev;
	}
}


//Die folgenden Zeilen nicht bearbeiten. Sie werden vom Klassen-Assistenten benötigt.
/*
BEGIN_MESSAGE_MAP(CTransferSocket, CAsyncSocketEx)
	//{{AFX_MSG_MAP(CTransferSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
*/

/////////////////////////////////////////////////////////////////////////////
// Member-Funktion CTransferSocket

void CTransferSocket::OnSend(int nErrorCode)
{
	CAsyncSocketEx::OnSend(nErrorCode);
	if (nErrorCode)
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
			CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		m_status=1;
		m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		Close();
		return;
	}

	if (m_nMode==TRANSFERMODE_LIST || m_nMode==TRANSFERMODE_NLST)
	{ //Send directory listing
		if (!m_bStarted)
			if (!InitTransfer(TRUE))
				return;
		while (m_pDirListing && m_pDirListing->len)
		{
			int numsend = m_nBufSize;
			if ((m_pDirListing->len - m_nBufferPos) < m_nBufSize)
				numsend = m_pDirListing->len - m_nBufferPos;

			int nLimit = m_pOwner->GetSpeedLimit(0);
			if (nLimit != -1 && GetState() != aborted && numsend > nLimit)
				numsend = nLimit;

			if (!numsend)
				return;

			int numsent = Send(m_pDirListing->buffer + m_nBufferPos, numsend);
			if (numsent==SOCKET_ERROR)
			{
				if (GetLastError()!=WSAEWOULDBLOCK)
				{
					Close();
					m_status=1;
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				}
				return;
			}

			if (nLimit != -1 && GetState() != aborted)
				m_pOwner->m_SlQuota.nDownloaded += numsent;

			((CServerThread *)m_pOwner->m_pOwner)->IncSendCount(numsent);
			GetSystemTime(&m_LastActiveTime);
			if (numsent < numsend)
				m_nBufferPos += numsent;
			else
				m_nBufferPos += numsend;

			ASSERT(m_nBufferPos <= m_pDirListing->len);

			if (m_nBufferPos == m_pDirListing->len)
			{
				t_dirlisting *pPrev = m_pDirListing;
				m_pDirListing = m_pDirListing->pNext;
				delete pPrev;
				m_nBufferPos = 0;

				if (!m_pDirListing)
					break;
			}

			//Check if there are other commands in the command queue.
			MSG msg;
			if (PeekMessage(&msg,0, 0, 0, PM_NOREMOVE))
			{
				TriggerEvent(FD_WRITE);
				return;
			}
		}
#ifndef NOLAYERS
		if (m_pGssLayer)
			if (!ShutDown() && GetLastError() == WSAEWOULDBLOCK)
				return;
#endif
		Close();
		m_status = 0;
		m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
	}
	else if (m_nMode==TRANSFERMODE_SEND)
	{ //Send file
		if (!m_bStarted)
			if (!InitTransfer(TRUE))
				return;
		int count=0;
		while (m_hFile!=INVALID_HANDLE_VALUE || m_nBufferPos)
		{
			count++;
			DWORD numread;
			if (m_nBufSize-m_nBufferPos && m_hFile!=INVALID_HANDLE_VALUE)
			{
				if (!ReadFile(m_hFile, m_pBuffer+m_nBufferPos, m_nBufSize-m_nBufferPos, &numread, 0))
				{
					CloseHandle(m_hFile);
					m_hFile = INVALID_HANDLE_VALUE;
					Close();
					m_status=3; //TODO: Better reason
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
					return;
				}

				if (!numread)
				{
					CloseHandle(m_hFile);
					m_hFile = INVALID_HANDLE_VALUE;
          if (!m_nBufferPos)
          { // only close if we've actually finished!
					  m_status=0;
					  m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
					  Close();
					  return;
          }
				}
				numread+=m_nBufferPos;
				m_nBufferPos=0;
			}
			else
				numread=m_nBufferPos;
			m_nBufferPos=0;

			if (numread<m_nBufSize)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}

			int numsend = numread;
			int nLimit = m_pOwner->GetSpeedLimit(0);
			if (nLimit != -1 && GetState() != aborted && numsend > nLimit)
				numsend = nLimit;

			if (!numsend)
			{
				m_nBufferPos = numread;
				return;
			}

			int numsent=Send(m_pBuffer, numsend);
			if (numsent==SOCKET_ERROR)
			{
				if (GetLastError()!=WSAEWOULDBLOCK)
				{
					CloseHandle(m_hFile);
					m_hFile = INVALID_HANDLE_VALUE;
					m_status=1;
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
					Close();
					return;
				}
				m_nBufferPos=numread;
				return;
			}
			else if ((unsigned int)numsent<numread)
			{
				memmove(m_pBuffer, m_pBuffer+numsent, numread-numsent);
				m_nBufferPos=numread-numsent;
			}

			if (nLimit != -1 && GetState() != aborted)
				m_pOwner->m_SlQuota.nDownloaded += numsent;

			((CServerThread *)m_pOwner->m_pOwner)->IncSendCount(numsent);
			GetSystemTime(&m_LastActiveTime);

			//Check if there are other commands in the command queue.
			MSG msg;
			if (PeekMessage(&msg,0, 0, 0, PM_NOREMOVE))
			{
				TriggerEvent(FD_WRITE);
				return;
			}
		}
#ifndef NOLAYERS
		if (m_pGssLayer)
			if (!ShutDown() && GetLastError() == WSAEWOULDBLOCK)
				return;
#endif
		m_status=0;
		Sleep(0); //Give the system the possibility to relay the data
				  //If not using Sleep(0), GetRight for example can't receive the last chunk.
		m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		Close();
	}
}

void CTransferSocket::OnConnect(int nErrorCode)
{
	if (nErrorCode)
	{
		if (m_hFile!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		Close();
		if (!m_bSentClose)
		{
			m_bSentClose = TRUE;
			m_status = 2;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		}
		return;
	}

#ifndef NOLAYERS
	if (m_pGssLayer)
		VERIFY(AddLayer(m_pGssLayer));
#endif

	if (!m_bStarted)
		InitTransfer(FALSE);

	CAsyncSocketEx::OnConnect(nErrorCode);
}

void CTransferSocket::OnClose(int nErrorCode)
{
	if (nErrorCode)
	{
		Close();
		if (m_hFile)
		{
			FlushFileBuffers(m_hFile);
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		if (!m_bSentClose)
		{
			m_bSentClose=TRUE;
			m_status=1;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		}
		return;
	}
	if (m_bReady)
	{
		if (m_nMode==TRANSFERMODE_RECEIVE)
		{
			//Receive all data still waiting to be recieve
			_int64 pos=0;
			do
			{
				if (m_hFile != INVALID_HANDLE_VALUE)
					pos=GetPosition64(m_hFile);
				OnReceive(0);
				if (m_hFile != INVALID_HANDLE_VALUE)
					if (pos == GetPosition64(m_hFile))
						break; //Leave loop when no data was written to file
			} while (m_hFile != INVALID_HANDLE_VALUE); //Or file was closed
			Close();
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
#if defined(_XBOX)
				if (m_nBufferPos)
				{
					DWORD numwritten;
					WriteFile(m_hFile, m_pBuffer, m_nBufferPos, &numwritten, 0);
				}
#endif
				FlushFileBuffers(m_hFile);
#if defined(_XBOX)
				SetEndOfFile(m_hFile);
#endif
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}
			if (!m_bSentClose)
			{
				m_bSentClose=TRUE;
				m_status=0;
				m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			}
		}
		else
		{
			Close();
			m_status=(m_nMode==TRANSFERMODE_RECEIVE)?0:1;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		}
	}

	CAsyncSocketEx::OnClose(nErrorCode);
}

int CTransferSocket::GetStatus()
{
	return m_status;
}

void CTransferSocket::OnAccept(int nErrorCode)
{
	CAsyncSocketEx tmp;
	Accept(tmp);
	SOCKET socket=tmp.Detach();
	Close();
	Attach(socket);
	bAccepted=TRUE;

#ifndef NOLAYERS
	if (m_pGssLayer)
		VERIFY(AddLayer(m_pGssLayer));
#endif

	if (m_bReady)
		if (!m_bStarted)
			InitTransfer(FALSE);

	CAsyncSocketEx::OnAccept(nErrorCode);
}

void CTransferSocket::OnReceive(int nErrorCode)
{
	CAsyncSocketEx::OnReceive(nErrorCode);

	if (nErrorCode)
	{
		Close();
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}
		if (!m_bSentClose)
		{
			m_bSentClose=TRUE;
			m_status=3;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		}
		return;
	}

	GetSystemTime(&m_LastActiveTime);
	if (m_nMode==TRANSFERMODE_RECEIVE)
	{
		if (!m_bStarted)
			if (!InitTransfer(FALSE))
				return;

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			ASSERT(m_Filename!="");
#if defined(_XBOX)
      // this to handle fat-x limitations
      if (g_guiSettings.GetBool("servers.ftpautofatx"))
      {
        /*CUtil::ShortenFileName(m_Filename); // change! addme to new ports
        CStdString strFilename = CUtil::GetFileName(m_Filename);
        CStdString strPath;
        CUtil::GetDirectory(m_Filename,strPath);
        vector<CStdString> tokens;
        CUtil::Tokenize(strPath,tokens,"\\/");
        strPath = tokens.front();
        for (vector<CStdString>::iterator iter=tokens.begin()+1; iter != tokens.end(); ++iter)
        {
          CUtil::ShortenFileName(*iter);
          CUtil::RemoveIllegalChars(*iter);
          strPath += "\\"+*iter;
        }
        CUtil::RemoveIllegalChars(strFilename);
        m_Filename = strPath+"\\"+strFilename;*/
        CUtil::GetFatXQualifiedPath(m_Filename);
      }
			m_hFile = CreateFile(m_Filename, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
#else
			m_hFile = CreateFile(m_Filename, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
#endif

			if (m_hFile == INVALID_HANDLE_VALUE)
			{
				Close();
				if (!m_bSentClose)
				{
					m_bSentClose=TRUE;
					m_status=3;
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				}
				return;
			}

#if defined(_XBOX)
			LARGE_INTEGER size;
			size.QuadPart = m_nRest;
			VERIFY(SetFilePointerEx(m_hFile, size, NULL, FILE_BEGIN));
			SetEndOfFile(m_hFile);
			m_nBufferPos = 0;
			m_nAlign = (4096 - (m_nRest & 4095)) & 4095;

			if (m_pBuffer)
				free(m_pBuffer);
			// Xbox writes ide data in 128k blocks, so always try to write 128k of data at a time.
			// Uses a 160k buffer so there's a bit of overrun as it's more efficient to always try and read 32k+ from the socket.
			// Also allows realignment to page alignment when restarting.
			m_nBufSize = 160*1024;
      m_pBuffer = (char*)malloc(m_nBufSize);
			if (!m_pBuffer)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
				Close();
				if (!m_bSentClose)
				{
					m_bSentClose=TRUE;
					m_status=6;
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				}
			}
			m_nPreAlloc = 0;
		}
		int len = m_nBufSize - m_nBufferPos;
#else
			DWORD low=(DWORD)(m_nRest&0xFFFFFFFF);
			LONG high=(LONG)(m_nRest>>32);
			VERIFY(SetFilePointer(m_hFile, low, &high, FILE_BEGIN)!=0xFFFFFFFF || GetLastError()==NO_ERROR);
			SetEndOfFile(m_hFile);
		}

		if (!m_pBuffer)
			m_pBuffer = new char[m_nBufSize];

		int len = m_nBufSize;
#endif
		int nLimit = -1;
		if (GetState() != closed)
		{
			nLimit = m_pOwner->GetSpeedLimit(1);
			if (nLimit != -1 && GetState() != aborted && len > nLimit)
				len = nLimit;
		}

		if (!len)
			return;

#if defined(_XBOX)
		int numread = Receive(m_pBuffer + m_nBufferPos, len);
#else
		int numread = Receive(m_pBuffer, len);
#endif

		if (numread==SOCKET_ERROR)
		{
			if (GetLastError()!=WSAEWOULDBLOCK)
			{
				if (m_hFile!=INVALID_HANDLE_VALUE)
				{
					CloseHandle(m_hFile);
					m_hFile = INVALID_HANDLE_VALUE;
				}
				Close();
				if (!m_bSentClose)
				{
					m_bSentClose=TRUE;
					m_status=1;
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				}
			}
			Sleep(0);
			return;
		}
		if (!numread)
		{
			if (m_hFile != INVALID_HANDLE_VALUE)
			{
#if defined(_XBOX)
				if (m_nBufferPos)
				{
					DWORD numwritten;
					WriteFile(m_hFile, m_pBuffer, m_nBufferPos, &numwritten, 0);
				}
				FlushFileBuffers(m_hFile);
				SetEndOfFile(m_hFile);
#endif
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
			}
			Close();
			if (!m_bSentClose)
			{
				m_bSentClose=TRUE;
				m_status=0;
				m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			}
			return;
		}
		((CServerThread *)m_pOwner->m_pOwner)->IncRecvCount(numread);

		if (nLimit != -1 && GetState() != aborted)
			m_pOwner->m_SlQuota.nUploaded += numread;

#if defined(_XBOX)
		m_nBufferPos += numread;

		if (m_nBufferPos >= 128*1024 + m_nAlign)
		{
			if (!m_nPreAlloc)
			{
				// pre alloc the file on disk - makes writing much faster due to less FAT updates
				SetFilePointer(m_hFile, 32*1024*1024, 0, FILE_CURRENT);
				SetEndOfFile(m_hFile);
				SetFilePointer(m_hFile, -32*1024*1024, 0, FILE_CURRENT);
				m_nPreAlloc = 32*1024*1024 / (128*1024);
			}

			DWORD numwritten;
			if (!WriteFile(m_hFile, m_pBuffer, 128*1024 + m_nAlign, &numwritten, 0) || numwritten != 128*1024 + m_nAlign)
			{
				CloseHandle(m_hFile);
				m_hFile = INVALID_HANDLE_VALUE;
				Close();
				if (!m_bSentClose)
				{
					m_bSentClose=TRUE;
					m_status=3; //TODO: Better reason
					m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				}
				return;
			}
			else
			{
				if (m_nBufferPos > numwritten)
				{
					memmove(m_pBuffer, m_pBuffer + numwritten, m_nBufferPos - numwritten);
				}
				m_nBufferPos -= numwritten;
				--m_nPreAlloc;
				m_nAlign = 0;
			}
		}
#else
		DWORD numwritten;
		if (!WriteFile(m_hFile, m_pBuffer, numread, &numwritten, 0) || numwritten!=(unsigned int)numread)
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
			Close();
			if (!m_bSentClose)
			{
				m_bSentClose=TRUE;
				m_status=3; //TODO: Better reason
				m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			}
			return;
		}
#endif
	}
}

void CTransferSocket::PasvTransfer()
{
	if(bAccepted)
		if (!m_bStarted)
			InitTransfer(FALSE);
}

BOOL CTransferSocket::InitTransfer(BOOL bCalledFromSend)
{
	if (m_nMode==TRANSFERMODE_RECEIVE)
	{ //Uploads from client
		if (!m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_INFXP))
		{ //Check if the IP of the remote machine is valid
			CStdString OwnerIP,TransferIP;

			SOCKADDR_IN sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			int nSockAddrLen = sizeof(sockAddr);
			BOOL bResult = m_pOwner->GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				OwnerIP = inet_ntoa(sockAddr.sin_addr);

			memset(&sockAddr, 0, sizeof(sockAddr));
			nSockAddrLen = sizeof(sockAddr);
			bResult = GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				TransferIP = inet_ntoa(sockAddr.sin_addr);

			if (!m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_NOINFXPSTRICT))
			{
				OwnerIP.Left(OwnerIP.ReverseFind('.'));
				TransferIP.Left(OwnerIP.ReverseFind('.'));
			}
			if (OwnerIP != TransferIP && OwnerIP != "127.0.0.1" && TransferIP != "127.0.0.1")
			{
				m_status = 5;
				Close();
				m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				return FALSE;
			}
		}
		AsyncSelect(FD_READ|FD_CLOSE);
	}
	else
	{ //Send files or directory listing to client
		if (!m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_OUTFXP))
		{ //Check if remote IP is valid
			CStdString OwnerIP,TransferIP;

			SOCKADDR_IN sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			int nSockAddrLen = sizeof(sockAddr);
			BOOL bResult = m_pOwner->GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				OwnerIP = inet_ntoa(sockAddr.sin_addr);

			memset(&sockAddr, 0, sizeof(sockAddr));
			nSockAddrLen = sizeof(sockAddr);
			bResult = GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				TransferIP = inet_ntoa(sockAddr.sin_addr);

			if (!m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_NOOUTFXPSTRICT))
			{
				OwnerIP.Left(OwnerIP.ReverseFind('.'));
				TransferIP.Left(OwnerIP.ReverseFind('.'));
			}
			if (OwnerIP != TransferIP && OwnerIP != "127.0.0.1" && TransferIP != "127.0.0.1")
			{
				m_status = 5;
				Close();
				m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
				return FALSE;
			}
		}
		AsyncSelect(FD_WRITE|FD_CLOSE);
	}

	if (bAccepted)
	{
		CStdString str="150 Connection accepted";
		if (m_nRest)
			str.Format("150 Connection accepted, restarting at offset %I64d",m_nRest);
		m_pOwner->Send(str);
	}

	m_bStarted=TRUE;
	if (m_nMode==TRANSFERMODE_SEND)
	{
#if defined(_XBOX)
		if (m_pBuffer)
			free(m_pBuffer);
		// smaller read buffer than for writes to avoid having to do massive data moves
		m_nBufSize = 32*1024;
		m_pBuffer = (char*)malloc(m_nBufSize);
		if (!m_pBuffer)
		{
			m_status=6;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			Close();
			return FALSE;
		}
		ASSERT(m_Filename!="");
		m_hFile = CreateFile(m_Filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
#else
		if (m_pBuffer)
			delete [] m_pBuffer;
		m_pBuffer=new char[m_nBufSize];
		ASSERT(m_Filename!="");
		m_hFile = CreateFile(m_Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#endif
		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_status=3;
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			Close();
			return FALSE;
		}
		DWORD low=(DWORD)(m_nRest&0xFFFFFFFF);
		LONG high=(LONG)(m_nRest>>32);
		if (SetFilePointer(m_hFile, low, &high, FILE_BEGIN)==0xFFFFFFFF && GetLastError()!=NO_ERROR)
		{
			high=0;
			VERIFY(SetFilePointer(m_hFile, 0, &high, FILE_END)!=0xFFFFFFFF || GetLastError()==NO_ERROR);
		}

	}

	GetSystemTime(&m_LastActiveTime);
	return TRUE;
}

BOOL CTransferSocket::CheckForTimeout()
{
	if (!m_bReady)
		return FALSE;

	_int64 timeout = m_pOwner->m_pOwner->m_pOptions->GetOptionVal(OPTION_TIMEOUT);

	SYSTEMTIME sCurrentTime;
	GetSystemTime(&sCurrentTime);
	FILETIME fCurrentTime;
	SystemTimeToFileTime(&sCurrentTime, &fCurrentTime);
	FILETIME fLastTime;
	SystemTimeToFileTime(&m_LastActiveTime, &fLastTime);
	_int64 elapsed = ((_int64)(fCurrentTime.dwHighDateTime - fLastTime.dwHighDateTime) << 32) + fCurrentTime.dwLowDateTime - fLastTime.dwLowDateTime;
	if (timeout && elapsed > (timeout*10000000))
	{
		m_status=4;
		m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
	}
	else if (!m_bStarted && elapsed > (10 * 10000000))
	{
		m_status = 2;
		m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
		return FALSE;
	}
	return TRUE;
}

BOOL CTransferSocket::Started() const
{
	return m_bStarted;
}

int CTransferSocket::GetMode() const
{
	return m_nMode;
}

#ifndef NOLAYERS
void CTransferSocket::UseGSS(CAsyncGssSocketLayer *pGssLayer)
{
	m_pGssLayer = new CAsyncGssSocketLayer;
	m_pGssLayer->InitTransferChannel(pGssLayer);
}

int CTransferSocket::OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nParam1, int nParam2)
{
	if (m_pGssLayer && pLayer == m_pGssLayer)
	{
		if (nType == LAYERCALLBACK_LAYERSPECIFIC && nParam1 == GSS_SHUTDOWN_COMPLETE)
		{
			m_status=0;
			Sleep(0); //Give the system the possibility to relay the data
			//If not using Sleep(0), GetRight for example can't receive the last chunk.
			m_pOwner->m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_TRANSFERMSG, m_pOwner->m_userid);
			Close();
			return 0;
		}
	}
	return CAsyncSocketEx::OnLayerCallback(pLayer, nType, nParam1, nParam2);
}
#endif
