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

// Server.cpp: Implementierung der Klasse CServer.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Server.h"
#include "Options.h"
#include "ServerThread.h"
#include "ListenSocket.h"
#include "AdminListenSocket.h"
#include "AdminInterface.h"
#include "AdminSocket.h"
#include "Permissions.h"
#include "FileLogger.h"
#include "version.h"

#if defined(_XBOX)
#include "utils/Log.h"
#endif

#ifndef MB_SERVICE_NOTIFICATION
#define MB_SERVICE_NOTIFICATION          0x00040000L
#endif

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CServer::CServer()
{
	m_hWnd=0;
	m_pListenSocket = NULL;
	m_pOptions = NULL;
	m_pFileLogger = NULL;
	m_pAdminInterface = new CAdminInterface(this);
	m_nServerState = 0;
	m_bQuit = FALSE;

	m_nSendCount = m_nRecvCount = 0;
	m_nTimerID = 0;
}

CServer::~CServer()
{
	if (m_pListenSocket)
		delete m_pListenSocket;
	m_pListenSocket = NULL;

	for (std::list<CAdminListenSocket*>::iterator iter = m_AdminListenSocketList.begin(); iter!=m_AdminListenSocketList.end(); iter++)
		delete *iter;
	m_AdminListenSocketList.clear();

	if (m_pAdminInterface)
		delete m_pAdminInterface;
	m_pAdminInterface=NULL;

	if (m_pFileLogger)
		delete m_pFileLogger;
	m_pFileLogger=NULL;

	if (m_pOptions)
		delete m_pOptions;
	m_pOptions=NULL;

	//Destroy window
	if (m_hWnd)
	{
		hMainWnd = 0;
		DestroyWindow(m_hWnd);
		m_hWnd=0;
	}
}

bool CServer::Create()
{
	//Create window
	WNDCLASSEX wndclass; 
	wndclass.cbSize=sizeof wndclass; 
	wndclass.style=0; 
	wndclass.lpfnWndProc=WindowProc; 
	wndclass.cbClsExtra=0; 
	wndclass.cbWndExtra=0; 
	wndclass.hInstance=GetModuleHandle(0); 
	wndclass.hIcon=0; 
	wndclass.hCursor=0; 
	wndclass.hbrBackground=0; 
	wndclass.lpszMenuName=0; 
	wndclass.lpszClassName=_T("FileZilla Server Helper Window"); 
	wndclass.hIconSm=0; 
	
	RegisterClassEx(&wndclass);
	
	m_hWnd=CreateWindow(_T("FileZilla Server Helper Window"), _T("FileZilla Server Helper Window"), 0, 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(0));
	if (!m_hWnd)
		return false;
	SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);

	hMainWnd = m_hWnd;

	m_pOptions=new COptions;
	m_pFileLogger = new CFileLogger(m_pOptions);
	
	//Create the threads
	int num = (int)m_pOptions->GetOptionVal(OPTION_THREADNUM);
	for (int i=0;i<num;i++)
	{
		CServerThread *pThread = new CServerThread;
		if (pThread->Create(THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED))
		{
			pThread->ResumeThread();
			m_ThreadArray.push_back(pThread);
		}
	}
	
	m_pFileLogger->Log(GetVersionString() + " started");
	m_pFileLogger->Log("Initializing Server.");
	
	m_pListenSocket = new CListenSocket(this);
	m_pListenSocket->m_pThreadList = &m_ThreadArray;
	
	//TODO Start on startup check
	int nPort = (int)m_pOptions->GetOptionVal(OPTION_SERVERPORT);
	CStdString str;
	str.Format("Creating listen socket on port %d...", nPort);
	ShowStatus(str, 0);
	if (!m_pListenSocket->Create(nPort, SOCK_STREAM, FD_ACCEPT, 0) || !m_pListenSocket->Listen())
	{
		str.Format("Failed to create listen socket on port %d. Server is not online!", nPort);
		ShowStatus(str, 1);
		delete m_pListenSocket;
		m_pListenSocket = NULL;
	}
	else
	{
		str.Format("Server online.");
		ShowStatus(str, 0);
		m_nServerState = 1;
	}

	m_nTimerID = SetTimer(m_hWnd, 1234, 10000, NULL);
	ASSERT(m_nTimerID);

	CreateAdminListenSocket();

	return true;
}

HWND CServer::GetHwnd()
{
	return m_hWnd;
}

LRESULT CALLBACK CServer::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CServer *pServer=(CServer *)GetWindowLong(hWnd, GWL_USERDATA);

	if (message == WM_CLOSE)
	{
		pServer->OnClose();
		return 0;
	}
	else if (message == WM_FILEZILLA_SERVERMSG)
		return pServer->OnServerMessage(wParam, lParam);
	else if (hWnd && message == WM_DESTROY)
	{
		ASSERT( hWnd == pServer->m_hWnd);
		HANDLE *handle=new HANDLE[pServer->m_ThreadArray.size()];
		unsigned int i=0;
		for (std::list<CServerThread *>::iterator iter=pServer->m_ThreadArray.begin(); iter!=pServer->m_ThreadArray.end(); iter++, i++)
		{
			handle[i]=(*iter)->m_hThread;
			(*iter)->PostThreadMessage(WM_QUIT, 0, 0);
		}
		for (i=0; i<pServer->m_ThreadArray.size(); i++)
		{
			int res=WaitForSingleObject(handle[i], 2000); // wait at most 2 seconds
			if (res==WAIT_FAILED)
				res=GetLastError();
			else if (res == WAIT_TIMEOUT)
			{
			  CLog::Log(LOGWARNING, "FileZilla failed to end a thread within 2000 msec, possible deadlock, but will continue anyway.");
			}
		}
		delete [] handle;
		for (std::list<CAdminListenSocket*>::iterator iter2 = pServer->m_AdminListenSocketList.begin(); iter2!=pServer->m_AdminListenSocketList.end(); iter2++)
		{
			(*iter2)->Close();
			delete *iter2;
		}
		pServer->m_AdminListenSocketList.clear();
		delete pServer->m_pAdminInterface;
		pServer->m_pAdminInterface = NULL;
		delete pServer->m_pOptions;
		pServer->m_pOptions = NULL;
		if (pServer->m_nTimerID)
			KillTimer(pServer->m_hWnd, pServer->m_nTimerID);
		PostQuitMessage(0);
		return 0;
	}
	else if ( message == WM_TIMER)
		pServer->OnTimer(wParam);
	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CServer::OnServerMessage(WPARAM wParam, LPARAM lParam)
{
	if (wParam==FSM_STATUSMESSAGE)
	{
		t_statusmsg *msg=reinterpret_cast<t_statusmsg *>(lParam);
		CStdString str;

		FILETIME fFileTime;
		SystemTimeToFileTime(&msg->time, &fFileTime);
		_int64 time = ((_int64)fFileTime.dwHighDateTime<<32) + fFileTime.dwLowDateTime;
		
		str.Format("(%06d)- %s (%s)> %s", msg->userid, (LPCTSTR)msg->user, (LPCTSTR)msg->ip, (LPCTSTR)msg->status);
		ShowStatus(time, str, msg->type);
		delete [] msg->user;
		delete [] msg->status;
		delete msg;
	}
	else if (wParam==FSM_CONNECTIONDATA)
	{
		t_connop *pConnOp = reinterpret_cast<t_connop*>(lParam);
		if (pConnOp->op != USERCONTROL_CONNOP_REMOVE)
			m_UsersList[pConnOp->data->userid] = *pConnOp->data;
		else
		{
			std::map<int, t_connectiondata>::iterator iter=m_UsersList.find(pConnOp->data->userid);
			if (iter!=m_UsersList.end())
				m_UsersList.erase(iter);
		}
		USES_CONVERSION;
		int userlen = pConnOp->data->user ? strlen(pConnOp->data->user) : 0;
		int len = 2 + 4 + strlen(pConnOp->data->ip)+2 + 4 + userlen+2;
		unsigned char *buffer = new unsigned char[len];
		buffer[0] = USERCONTROL_CONNOP;
		buffer[1] = pConnOp->op;
		memcpy(buffer+2, &pConnOp->data->userid, 4);
		buffer[2 + 4] = strlen(pConnOp->data->ip)/256;
		buffer[2 + 4 + 1] = strlen(pConnOp->data->ip)%256;
		memcpy(buffer + 2 + 4 + 2, T2CA(pConnOp->data->ip), strlen(T2CA(pConnOp->data->ip)));
		memcpy(buffer + 2 + 4 +2 + strlen(pConnOp->data->ip)+2, &pConnOp->data->port, 4);
		buffer[2 + 4 + strlen(pConnOp->data->ip)+2 + 4]= userlen/256;
		buffer[2 + 4 + strlen(pConnOp->data->ip)+2 + 4 + 1]= userlen%256;
		if (pConnOp->data->user)
			memcpy(buffer + 2 + 4 + strlen(pConnOp->data->ip)+2 + 4 + 2, T2CA(pConnOp->data->user), userlen);
		m_pAdminInterface->SendCommand(2, 3, buffer, len);
		delete [] buffer;
		delete pConnOp->data;
		delete pConnOp;
	}
	else if (wParam==FSM_THREADCANQUIT)
	{
		CServerThread *pThread=(CServerThread *)lParam;
		for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++)
		{
			if (*iter==pThread)
			{
				HANDLE handle=pThread->m_hThread;
				pThread->PostThreadMessage(WM_QUIT, 0, 0);
				int res=WaitForSingleObject(handle, INFINITE);
				if (res==WAIT_FAILED)
					res=GetLastError();
				m_ThreadArray.erase(iter);
				if (!m_ThreadArray.size())
				{
					if (!m_bQuit)
						ShowStatus(_T("Server offline."), 1);
					else
						DestroyWindow(m_hWnd);
				}
				break;
			}
			
		}
	}
	else if (wParam==FSM_SEND)
	{
		char buffer[5];
		buffer[0] = 1;
		memcpy(buffer+1, &lParam, 4);
		m_pAdminInterface->SendCommand(2, 7, buffer, 5);
		m_nSendCount += lParam;
	}
	else if (wParam==FSM_RECV)
	{
		char buffer[5];
		buffer[0] = 0;
		memcpy(buffer+1, &lParam, 4);
		m_pAdminInterface->SendCommand(2, 7, buffer, 5);
		m_nRecvCount += lParam;
	}
	else if (wParam == FSM_RELOADCONFIG)
	{
		COptions options;
		options.ReloadConfig();
		CPermissions perm;
		perm.ReloadConfig();
	}
	return 0;
}


void CServer::OnClose()
{
	if (m_pListenSocket)
	{
		m_pListenSocket->Close();
		delete m_pListenSocket;
		m_pListenSocket = NULL;
	}
	m_bQuit = TRUE;
	for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++)
	{
		VERIFY((*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 0));
	}
}

BOOL CServer::ProcessCommand(CAdminSocket *pAdminSocket, int nID, unsigned char *pData, int nDataLength)
{
	switch (nID)
	{
	case 2:
		if (!nDataLength)
		{
			unsigned char buffer[2];
			buffer[0]=m_nServerState/256;
			buffer[1]=m_nServerState%256;
			pAdminSocket->SendCommand(1, 2, buffer, 2);
		}
		else if (nDataLength == 2)
		{
			ToggleActive(*pData*256+pData[1]);
			unsigned char buffer[2];
			buffer[0]=m_nServerState/256;
			buffer[1]=m_nServerState%256;
			pAdminSocket->SendCommand(1, 2, buffer, 2);
		}
		else
			pAdminSocket->SendCommand(1, 1, "\001Protocol error: Unexpected data length", strlen("\001Protocol error: Unexpected data length")+1);
		break;
	case 3:
		if (!nDataLength)
			pAdminSocket->SendCommand(1, 1, "\001Protocol error: Unexpected data length", strlen("\001Protocol error: Unexpected data length")+1);
		else if (*pData == USERCONTROL_GETLIST)
		{
			int len = 3;
			std::map<int, t_connectiondata>::iterator iter;
			for (iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
				len += 4 + strlen(iter->second.ip)+2 + 4 + strlen(iter->second.user)+2;
			unsigned char *buffer = new unsigned char[len];
			buffer[0] = USERCONTROL_GETLIST;
			buffer[1] = m_UsersList.size() / 256;
			buffer[2] = m_UsersList.size() % 256;
			unsigned char *p = buffer + 3;
			for (iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
			{
				USES_CONVERSION;
				memcpy(p, &iter->second.userid, 4);
				p+=4;
				*p++ = strlen(iter->second.ip) / 256;
				*p++ = strlen(iter->second.ip) % 256;
				memcpy(p, T2CA(iter->second.ip), strlen(T2CA(iter->second.ip)));
				p+=strlen(T2CA(iter->second.ip));

				memcpy(p, &iter->second.port, 4);
				p+=4;

				if (iter->second.user)
				{
					*p++ = strlen(iter->second.user) / 256;
					*p++ = strlen(iter->second.user) % 256;
					memcpy(p, T2CA(iter->second.user), strlen(T2CA(iter->second.user)));
					p+=strlen(T2CA(iter->second.user));
				}
				else
				{
					*p++ = 0;
					*p++ = 0;
				}
				
			}
			m_pAdminInterface->SendCommand(1, 3, buffer, len);			
			delete [] buffer;
		}
		else if (*pData == USERCONTROL_KICK)
		{
			if (nDataLength != 5)
				pAdminSocket->SendCommand(1, 1, "\001Protocol error: Unexpected data length", strlen("\001Protocol error: Unexpected data length")+1);
			else
			{
				int nUserID;
				memcpy(&nUserID, pData+1, 4);
				
				std::map<int, t_connectiondata>::iterator iter = m_UsersList.find(nUserID);
				if (iter!=m_UsersList.end())
				{
					t_controlmessage *msg=new t_controlmessage;
					msg->command=USERCONTROL_KICK;
					msg->socketid=nUserID;
					iter->second.pThread->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_CONTROL, (LPARAM)msg);
					char buffer[2];
					buffer[0] = USERCONTROL_KICK;
					buffer[1] = 0;
					pAdminSocket->SendCommand(1, 3, &buffer, 2);
				}
				else
				{
					char buffer[2];
					buffer[0] = USERCONTROL_KICK;
					buffer[1] = 1;
					pAdminSocket->SendCommand(1, 3, &buffer, 2);
				}
			}
		}
		else
			pAdminSocket->SendCommand(1, 1, "\001Protocol error: Invalid data", strlen("\001Protocol error: Invalid data")+1);
		break;
	case 5:
		if (!nDataLength)
		{
			char *pBuffer = NULL;
			DWORD nBufferLength = 0;
			if (m_pOptions && m_pOptions->GetAsCommand(&pBuffer, &nBufferLength))
			{
				pAdminSocket->SendCommand(1, 5, pBuffer, nBufferLength);
				delete [] pBuffer;
			}
		}
		else if (m_pOptions)
		{
			if (nDataLength < 2)
				pAdminSocket->SendCommand(1, 1, "\001Protocol error: Unexpected data length", strlen("\001Protocol error: Unexpected data length")+1);
			else
			{
				int nListenPort = (int)m_pOptions->GetOptionVal(OPTION_SERVERPORT);
				int nAdminListenPort = (int)m_pOptions->GetOptionVal(OPTION_ADMINPORT);
				CStdString adminIpBindings = m_pOptions->GetOption(OPTION_ADMINIPBINDINGS);
				
				SOCKADDR_IN sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				int nSockAddrLen = sizeof(sockAddr);	
				BOOL bLocal = pAdminSocket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen) && sockAddr.sin_addr.S_un.S_addr == 0x0100007f;

				if (!m_pOptions->ParseOptionsCommand(pData, nDataLength, bLocal))
				{
					pAdminSocket->SendCommand(1, 1, "\001Protocol error: Invalid data, could not import settings.", strlen("\001Protocol error: Invalid data, could not import settings.")+1);
					char buffer = 1;
					pAdminSocket->SendCommand(1, 5, &buffer, 1);
					break;
				}

				char buffer = 0;
				pAdminSocket->SendCommand(1, 5, &buffer, 1);

				unsigned int threadnum = (int)m_pOptions->GetOptionVal(OPTION_THREADNUM);
				if (threadnum>m_ThreadArray.size())
				{
					int newthreads=threadnum-m_ThreadArray.size();
					for (int i=0;i<newthreads;i++)
					{
						CServerThread *pThread = new CServerThread;
						if (pThread->Create(THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED))
						{
							pThread->ResumeThread();
							m_ThreadArray.push_back(pThread);
						}
					}
					CStdString str;
					str.Format("Number of threads increased to %d.", threadnum);
					ShowStatus(str, 0);
				}
				else if (threadnum<m_ThreadArray.size())
				{
					CStdString str;
					str.Format("Decreasing number of threads to %d.", threadnum);
					ShowStatus(str, 0);
					unsigned int i=0;
					for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++,i++)
						if (i>=threadnum)
							(*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 2);
				}
				if (nListenPort != m_pOptions->GetOptionVal(OPTION_SERVERPORT))
				{
					if (m_pListenSocket)
					{
						CStdString str;
						str.Format("Closing listen socket on port %d", nListenPort);
						ShowStatus(str, 0);
						m_pListenSocket->Close();
						str.Format("Creating listen socket on port %I64d...", m_pOptions->GetOptionVal(OPTION_SERVERPORT));
						ShowStatus(str, 0);
						if (!m_pListenSocket->Create((int)m_pOptions->GetOptionVal(OPTION_SERVERPORT), SOCK_STREAM,FD_ACCEPT,0) || !m_pListenSocket->Listen())
						{
							delete m_pListenSocket;
							m_pListenSocket = NULL;
							str.Format("Failed to create listen socket on port %I64d. Server is not online!", m_pOptions->GetOptionVal(OPTION_SERVERPORT));
							ShowStatus(str,1);
							m_nServerState = 0;
						}
						else
							ShowStatus("Listen socket port changed",0);
					}
				}
				if (nAdminListenPort != m_pOptions->GetOptionVal(OPTION_ADMINPORT) || adminIpBindings!=m_pOptions->GetOption(OPTION_ADMINIPBINDINGS))
				{
					if (nAdminListenPort == m_pOptions->GetOptionVal(OPTION_ADMINPORT))
					{
						for (std::list<CAdminListenSocket*>::iterator iter = m_AdminListenSocketList.begin(); iter!=m_AdminListenSocketList.end(); iter++)
						{
							(*iter)->Close();
							delete *iter;
						}
						m_AdminListenSocketList.clear();
					}
					CAdminListenSocket *pSocket = new CAdminListenSocket(m_pAdminInterface);
					if (!pSocket->Create((int)m_pOptions->GetOptionVal(OPTION_ADMINPORT), SOCK_STREAM, FD_ACCEPT, (m_pOptions->GetOption(OPTION_ADMINIPBINDINGS)!="*") ? _T("127.0.0.1") : NULL))
					{
						delete pSocket;
						CStdString str;
						str.Format(_T("Failed to change admin listen port to %I64d."), m_pOptions->GetOptionVal(OPTION_ADMINPORT));
						m_pOptions->SetOption(OPTION_ADMINPORT, nAdminListenPort);
						ShowStatus(str, 1);
					}
					else
					{
						pSocket->Listen();
						for (std::list<CAdminListenSocket*>::iterator iter = m_AdminListenSocketList.begin(); iter!=m_AdminListenSocketList.end(); iter++)
						{
							(*iter)->Close();
							delete *iter;
						}
						m_AdminListenSocketList.clear();

						m_AdminListenSocketList.push_back(pSocket);
						if (nAdminListenPort != m_pOptions->GetOptionVal(OPTION_ADMINPORT))
						{
							CStdString str;
							str.Format(_T("Admin listen port changed to %I64d."), m_pOptions->GetOptionVal(OPTION_ADMINPORT));
							ShowStatus(str, 0);
						}

						if (m_pOptions->GetOption(OPTION_ADMINIPBINDINGS) != "*")
						{
							BOOL bError = FALSE;
							CStdString str = _T("Failed to bind the admin interface to the following IPs:");
							CStdString ipBindings = m_pOptions->GetOption(OPTION_ADMINIPBINDINGS);

							if (ipBindings != "")
								ipBindings += " ";
							while (ipBindings != "")
							{
								int pos = ipBindings.Find(" ");
								if (pos == -1)
									break;
								CStdString ip = ipBindings.Left(pos);
								ipBindings = ipBindings.Mid(pos+1);
								CAdminListenSocket *pAdminListenSocket = new CAdminListenSocket(m_pAdminInterface);
								if (!pAdminListenSocket->Create((int)m_pOptions->GetOptionVal(OPTION_ADMINPORT), SOCK_STREAM, FD_ACCEPT, ip) || !pAdminListenSocket->Listen())
								{
									bError = TRUE;
									str += _T(" ") + ip;
									delete pAdminListenSocket;
								}
								else
									m_AdminListenSocketList.push_back(pAdminListenSocket);
							}
							if (bError)
								ShowStatus(str, 1);
						}
						if (adminIpBindings!=m_pOptions->GetOption(OPTION_ADMINIPBINDINGS))
							ShowStatus(_T("Admin interface IP bindings changed"), 0);
					}
	
				}
			}
		}
		break;
	case 6:
		if (!nDataLength)
		{
			char *pBuffer = NULL;
			DWORD nBufferLength = 0;
			CPermissions permissions;
			permissions.GetAsCommand(&pBuffer, &nBufferLength);
			pAdminSocket->SendCommand(1, 6, pBuffer, nBufferLength);
			delete [] pBuffer;
		}
		else
		{
			if (nDataLength < 2)
				pAdminSocket->SendCommand(1, 1, "\001Protocol error: Unexpected data length", strlen("\001Protocol error: Unexpected data length")+1);
			else
			{
				CPermissions permissions;
				if (!permissions.ParseUsersCommand(pData, nDataLength))
				{
					pAdminSocket->SendCommand(1, 1, "\001Protocol error: Invalid data, could not import account settings.", strlen("\001Protocol error: Invalid data, could not import account settings.")+1);
					char buffer = 1;
					pAdminSocket->SendCommand(1, 6, &buffer, 1);
					break;
				}
				char buffer = 0;
				pAdminSocket->SendCommand(1, 6, &buffer, 1);
			}
		}
		break;
	case 8:
		pAdminSocket->SendCommand(1, 8, NULL, 0);
		break;
	default:
		{
			CStdString str;
			str.Format("\001Protocol error: Unknown command (%d).", nID);
			pAdminSocket->SendCommand(1, 1, str.c_str(), str.GetLength());
		}
		break;
	}

	return TRUE;
}

BOOL CServer::ToggleActive(int nServerState)
{
	if (nServerState&2)
	{
		if (m_pListenSocket)
		{
			m_pListenSocket->Close();
			delete m_pListenSocket;
			m_pListenSocket = NULL;
		}
		for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++)
			(*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 0);			
		if (m_nServerState & 1)
			m_nServerState = m_nServerState |= 2;
	}
	else if (nServerState&4)
	{
		if (m_pListenSocket)
		{
			m_pListenSocket->Close();
			delete m_pListenSocket;
			m_pListenSocket = NULL;
		}
		for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++)
			(*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 2);			
		if (m_nServerState & 1)
			m_nServerState = m_nServerState |= 4;
	}
	else if (nServerState&8)
	{
		if (m_pListenSocket)
		{
			m_pListenSocket->Close();
			delete m_pListenSocket;
			m_pListenSocket = NULL;
		}
		for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++)
			(*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 1);			
		if (m_nServerState & 1)
			m_nServerState = m_nServerState |= 8;
	}
	else if (nServerState&1)
	{
		if (!m_pListenSocket)
		{
			m_pListenSocket = new CListenSocket(this);
			m_pListenSocket->m_pThreadList=&m_ThreadArray;
			
			int nPort = (m_pOptions ? (int)m_pOptions->GetOptionVal(OPTION_SERVERPORT) : 21);
			CStdString str;
			str.Format("Creating listen socket on port %d...", nPort);
			ShowStatus(str, 0);
			if (!m_pListenSocket->Create(nPort, SOCK_STREAM, FD_ACCEPT, 0) || !m_pListenSocket->Listen())
			{
				delete m_pListenSocket;
				m_pListenSocket = NULL;
				str.Format("Failed to create listen socket on port %d. Server is not online!", nPort);
				ShowStatus(str, 1);
			}
			else
			{
				ShowStatus(_T("Server online"), 0);
			}
		}

		if (m_pListenSocket)
		{
			int i=0;
			int num = (m_pOptions ? (int)m_pOptions->GetOptionVal(OPTION_THREADNUM) : 2);
			for (std::list<CServerThread *>::iterator iter=m_ThreadArray.begin(); iter!=m_ThreadArray.end(); iter++, i++)
				if (i<num)
					(*iter)->DontQuit();
				else
					break;
			//Recreate the threads
			for (i=m_ThreadArray.size();i<num;i++)
			{
				CServerThread *pThread = new CServerThread;
				if (pThread->Create(THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED))
				{
					pThread->ResumeThread();
					m_ThreadArray.push_back(pThread);
				}
			}
		}
		m_pListenSocket->m_bLocked = nServerState & 0x10;

		m_nServerState = (m_pListenSocket?1:0) + (nServerState&16);
	}
	unsigned char buffer[2];
	buffer[0]=m_nServerState/256;
	buffer[1]=m_nServerState%256;
	m_pAdminInterface->SendCommand(2, 2, buffer, 2);
	return TRUE;
}

void CServer::ShowStatus(LPCTSTR msg, int nType)
{
	USES_CONVERSION;
	LPCSTR str=T2CA(msg);
	char *pBuffer=new char[strlen(str)+1];
	*pBuffer=nType;
	memcpy(pBuffer+1, str, strlen(str));
	if (m_pAdminInterface)
		m_pAdminInterface->SendCommand(2, 1, pBuffer, strlen(str)+1);
	delete [] pBuffer;

	if (m_pFileLogger)
		m_pFileLogger->Log(msg);
}

void CServer::ShowStatus(_int64 eventDate, LPCTSTR msg, int nType)
{
	USES_CONVERSION;
	TIME_ZONE_INFORMATION tzInfo;
	BOOL res=GetTimeZoneInformation(&tzInfo);
	_int64 offset = tzInfo.Bias+((res==TIME_ZONE_ID_DAYLIGHT)?tzInfo.DaylightBias:tzInfo.StandardBias);
	offset*=60*10000000;
	eventDate-=offset;
	LPCSTR str=T2CA(msg);
	char *pBuffer=new char[strlen(str) + 1 + 8];
	*pBuffer=nType;
	memcpy(pBuffer + 1, &eventDate, 8);
	memcpy(pBuffer + 1 + 8, str, strlen(str));
	if (m_pAdminInterface)
		m_pAdminInterface->SendCommand(2, 4, pBuffer, strlen(str) + 1 + 8);
	delete [] pBuffer;

	//Log string
	if (m_pFileLogger)
	{
		FILETIME fFileTime;
		SYSTEMTIME sFileTime;
		fFileTime.dwHighDateTime = (DWORD)(eventDate>>32);
		fFileTime.dwLowDateTime = (DWORD)(eventDate %0xFFFFFFFF);
		FileTimeToSystemTime(&fFileTime, &sFileTime);
		char text[80];
		if (!GetDateFormat(
			LOCALE_USER_DEFAULT,               // locale for which date is to be formatted
			DATE_SHORTDATE,             // flags specifying function options
			&sFileTime,  // date to be formatted
			0,          // date format string
			text,          // buffer for storing formatted string
			80                // size of buffer
			))
			return;
		
		CStdString text2=" ";
		text2+=text;
		
		if (!GetTimeFormat(
			LOCALE_USER_DEFAULT,               // locale for which date is to be formatted
			TIME_FORCE24HOURFORMAT,             // flags specifying function options
			&sFileTime,  // date to be formatted
			0,          // date format string
			text,          // buffer for storing formatted string
			80                // size of buffer
			))
			return;

		text2+=" ";
		text2+=text;
		CStdString str = msg;
		int pos=str.Find("-");
		if (pos!=-1)
		{
			str.Insert(pos, text2 + " ");
		}
		m_pFileLogger->Log(str);
	}
}

void CServer::OnTimer(UINT nIDEvent)
{
	m_pAdminInterface->CheckForTimeout();
	m_pFileLogger->CheckLogFile();
}

BOOL CServer::CreateAdminListenSocket()
{
	CStdString ipBindings = (m_pOptions ? m_pOptions->GetOption(OPTION_ADMINIPBINDINGS) : "");
	int nAdminPort = (m_pOptions ? (int)m_pOptions->GetOptionVal(OPTION_ADMINPORT) : 14147);
	CAdminListenSocket *pAdminListenSocket = new CAdminListenSocket(m_pAdminInterface);
	BOOL bError = FALSE;
	if (!pAdminListenSocket->Create(nAdminPort, SOCK_STREAM, FD_ACCEPT, (ipBindings!="*") ? _T("127.0.0.1") : NULL))
	{
		CStdString str;
		if (!pAdminListenSocket->Create(0, SOCK_STREAM, FD_ACCEPT, _T("127.0.0.1")))
		{
			delete pAdminListenSocket;
			pAdminListenSocket = NULL;
			str.Format(_T("Failed to create listen socket for admin interface on port %d, the admin interface has been disabled."), nAdminPort);
		}
		else
		{
			SOCKADDR_IN sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			int nSockAddrLen = sizeof(sockAddr);
			BOOL bResult = pAdminListenSocket->GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
			{
				int nPort = ntohs(sockAddr.sin_port);
				str.Format(_T("Failed to create listen socket for admin interface on port %d, for this session the admin interface is available on port %d."), nAdminPort, nPort);
				nAdminPort = nPort;
			}
			else
			{
				delete pAdminListenSocket;
				pAdminListenSocket = NULL;
				str.Format(_T("Failed to create listen socket for admin interface on port %d, the admin interface has been disabled."), nAdminPort);
			}
		}
		MessageBox(0, str, _T("FileZilla Server Error"), MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);
	}
	if (pAdminListenSocket)
	{
		VERIFY(pAdminListenSocket->Listen());
		m_AdminListenSocketList.push_back(pAdminListenSocket);
	}

	if (!bError && ipBindings != "*")
	{
		BOOL bError = FALSE;
		CStdString str = _T("Failed to bind the admin interface to the following IPs:");
		if (ipBindings != "")
			ipBindings += " ";
		while (ipBindings != "")
		{
			int pos = ipBindings.Find(" ");
			if (pos == -1)
				break;
			CStdString ip = ipBindings.Left(pos);
			ipBindings = ipBindings.Mid(pos+1);
			CAdminListenSocket *pAdminListenSocket = new CAdminListenSocket(m_pAdminInterface);
			if (!pAdminListenSocket->Create(nAdminPort, SOCK_STREAM, FD_ACCEPT, ip) || !pAdminListenSocket->Listen())
			{
				delete pAdminListenSocket;
				bError = TRUE;
				str += _T("\n") + ip;
			}
			else
				m_AdminListenSocketList.push_back(pAdminListenSocket);
		}
		if (bError)
			MessageBox(0, str, _T("FileZilla Server Error"), MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);
	}
	return !m_AdminListenSocketList.empty();
}
