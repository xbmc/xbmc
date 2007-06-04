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

// ServerThread.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "ServerThread.h"
#include "ControlSocket.h"
#include "transfersocket.h"
#include "Options.h"
#include "version.h"
#include "Permissions.h"
#include "ExternalIpCheck.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

std::map<int, t_socketdata> CServerThread::m_userids;
CCriticalSectionWrapper CServerThread::m_GlobalThreadsync;
std::map<CStdString, int> CServerThread::m_userIPs;
std::list<CServerThread*> CServerThread::m_sInstanceList;

#define EGCS m_GlobalThreadsync.Lock()
#define LGCS m_GlobalThreadsync.Unlock()

/////////////////////////////////////////////////////////////////////////////
// CServerThread

CServerThread::CServerThread()
{
}

CServerThread::~CServerThread()
{
}

BOOL CServerThread::InitInstance()
{
	BOOL res=TRUE;
	WSADATA wsaData;
	
	WORD wVersionRequested = MAKEWORD(1, 1);
	int nResult = WSAStartup(wVersionRequested, &wsaData);
	if (nResult != 0)
		res=FALSE;
	else if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		res=FALSE;
	}
	
	m_timerid=SetTimer(0, 0, 1000, 0);
	m_nRateTimer=SetTimer(0, 0, 100, 0);
	m_bQuit=FALSE;
	m_nRecvCount=0;
	m_nSendCount=0;
	m_pOptions = new COptions;
	m_pPermissions = new CPermissions;

	EGCS;
	if (m_sInstanceList.empty())
		m_bIsMaster = TRUE;
	else
		m_bIsMaster = FALSE;
	m_sInstanceList.push_back(this);
	LGCS;

	m_nLoopCount = 0;

	m_threadsync.Lock();
	if (!m_bIsMaster)
		m_pExternalIpCheck = NULL;
	else
		m_pExternalIpCheck = new CExternalIpCheck(this);
	m_threadsync.Unlock();

	return TRUE;
}

DWORD CServerThread::ExitInstance()
{
	ASSERT(m_pPermissions);
	delete m_pPermissions;
	m_pPermissions=0;
	delete m_pOptions;
	m_pOptions=0;
	KillTimer(0, m_timerid);
	KillTimer(0, m_nRateTimer);
	WSACleanup();

	if (m_bIsMaster)
	{
		EGCS;
		m_sInstanceList.remove(this);
		if (!m_sInstanceList.empty())
			m_sInstanceList.front()->m_bIsMaster = TRUE;
		LGCS;

		delete m_pExternalIpCheck;
		m_pExternalIpCheck = NULL;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CServerThread 

const int CServerThread::GetNumConnections()
{
	m_threadsync.Lock();
	int num = m_LocalUserIDs.size();
	m_threadsync.Unlock();
	return num;
}

void CServerThread::AddSocket(SOCKET sockethandle)
{
	PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_NEWSOCKET, (LPARAM)sockethandle);
}

#define IDMAX 1000000000
int CServerThread::CalcUserID()
{
	if (m_userids.size() >= IDMAX)
		return -1;
	static int curid=0;
	curid++;
	if (curid==IDMAX)
		curid=1;
	while (m_userids.find(curid) != m_userids.end())
	{
		curid++;
		if (curid == IDMAX)
			curid=1;
	}
	return curid;
}

void CServerThread::AddNewSocket(SOCKET sockethandle)
{
	CControlSocket *socket = new CControlSocket(this);
	socket->Attach(sockethandle);
	CStdString ip;
	unsigned int port;
	
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	if (bResult)
	{
		port = ntohs(sockAddr.sin_port);
		ip = inet_ntoa(sockAddr.sin_addr);
	}
	else
	{
		socket->m_RemoteIP="ip unknown";
		socket->m_userid=0;
		socket->SendStatus("Can't get remote IP, disconnected",1);
		socket->Close();
		delete socket;
		return;
	}
	socket->m_RemoteIP=ip;
	EGCS;
	int userid = CalcUserID();
	if (userid == -1)
	{
		LGCS;
		socket->m_userid=0;
		socket->SendStatus("Refusing connection, server too busy!", 1);
		socket->Send("421 Server too busy, closing connection. Please retry later!");
		socket->Close();
		delete socket;
		return;
	}
	socket->m_userid = userid;
	t_socketdata data;
	data.pSocket = socket;
	data.pThread = this;
	m_userids[userid] = data;
	LGCS;
	m_threadsync.Lock();
	m_LocalUserIDs[userid] = socket;
	m_threadsync.Unlock();

	t_connectiondata *conndata = new t_connectiondata;
	t_connop *op = new t_connop;

  memset(conndata, 0, sizeof(t_connectiondata));
  memset(op, 0, sizeof(t_connop));

	op->data = conndata;
	op->op = USERCONTROL_CONNOP_ADD;
	conndata->userid = userid;
	conndata->pThread = this;

	memset(&sockAddr, 0, sizeof(sockAddr));
	nSockAddrLen = sizeof(sockAddr);
	bResult = socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	if (bResult)
	{
		conndata->port = ntohs(sockAddr.sin_port);
		strcpy(conndata->ip, inet_ntoa(sockAddr.sin_addr));
	}

	if (!PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_CONNECTIONDATA, (LPARAM)op))
	{
		delete op->data;
		delete op;
	}
	
	socket->AsyncSelect(FD_READ|FD_WRITE|FD_CLOSE);
	socket->SendStatus("Connected, sending welcome message...", 0);
	
	CStdString msg = m_pOptions->GetOption(OPTION_WELCOMEMESSAGE);
	if (m_RawWelcomeMessage != msg)
	{
		m_RawWelcomeMessage = msg;
		m_ParsedWelcomeMessage.clear();

		msg.Replace("%%", "\001");
		msg.Replace("%v", GetVersionString());
		msg.Replace("\001", "%");
	
		ASSERT(msg!="");
		int oldpos=0;
		msg.Replace("\r\n", "\n");
		int pos=msg.Find("\n");
		CStdString line;
		while (pos!=-1)
		{
		  // why is there an assertion here?
			// ASSERT(pos);
			m_ParsedWelcomeMessage.push_back("220-" +  msg.Mid(oldpos, pos-oldpos) );
			oldpos=pos+1;
			pos=msg.Find("\n", oldpos);
		}
	
		line = msg.Mid(oldpos);
		if (line != "")
			m_ParsedWelcomeMessage.push_back("220 " + line);		
		else
		{
			m_ParsedWelcomeMessage.back()[3] = 0;
		}
	}
	
	ASSERT(!m_ParsedWelcomeMessage.empty());
	for (std::list<CStdString>::iterator iter = m_ParsedWelcomeMessage.begin(); iter != m_ParsedWelcomeMessage.end(); iter++)
		if (!socket->Send(*iter))
			break;
}

int CServerThread::OnThreadMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_FILEZILLA_THREADMSG)
	{
		if (wParam == FTM_NEWSOCKET) //Add a new socket to this thread
			AddNewSocket((SOCKET)lParam);
		else if (wParam==FTM_DELSOCKET) //Remove a socket from this thread
		{
			EGCS;
			CControlSocket *socket=GetControlSocket(lParam);
			m_threadsync.Lock();
			if (m_LocalUserIDs.find(lParam)!=m_LocalUserIDs.end())
				m_LocalUserIDs.erase(m_LocalUserIDs.find(lParam));
			m_threadsync.Unlock();
			if (socket)
			{
				socket->Close();
				delete socket;
			}
			if (m_userids.find(lParam)!=m_userids.end())
				m_userids.erase(m_userids.find(lParam));
			LGCS;
			m_threadsync.Lock();
			if (m_bQuit)
			{
				int count=m_LocalUserIDs.size();
				if (!count)
					PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_THREADCANQUIT, (LPARAM)this);
			}
			m_threadsync.Unlock();
		}
		else if (wParam==FTM_COMMAND)
		{ //Process a command sent from a client
			CControlSocket *socket=GetControlSocket(lParam);
			if (socket)
				socket->ParseCommand();
		}
		else if (wParam==FTM_TRANSFERMSG)
		{
			CControlSocket *socket=GetControlSocket(lParam);
			if (socket)
				socket->ProcessTransferMsg();
		}
		else if (wParam==FTM_GOOFFLINE)
		{
			m_threadsync.Lock();
			m_bQuit=TRUE;
			int count=m_LocalUserIDs.size();
			if (!count)
			{
				m_threadsync.Unlock();
				PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_THREADCANQUIT, (LPARAM)this);
				return 0;
			}
			if (lParam==2)
			{
				m_threadsync.Unlock();
				return 0;
			}
			for (std::map<int, CControlSocket *>::iterator iter=m_LocalUserIDs.begin(); iter!=m_LocalUserIDs.end(); iter++)
			{
				if (!lParam)
					iter->second->ForceClose(0);
				else if (lParam==1)
					iter->second->WaitGoOffline();
			}
			m_threadsync.Unlock();
		}
		else if (wParam==FTM_CONTROL)
			ProcessControlMessage((t_controlmessage *)lParam);
	}
	else if (Msg == WM_TIMER)
		OnTimer(wParam, lParam);
	return 0;
}

void CServerThread::OnTimer(WPARAM wParam,LPARAM lParam)
{
	if (wParam==m_timerid)
	{
		m_threadsync.Lock();
		for (std::map<int, CControlSocket *>::iterator iter=m_LocalUserIDs.begin(); iter!=m_LocalUserIDs.end(); iter++)
			iter->second->CheckForTimeout();
		m_threadsync.Unlock();

		if (m_bIsMaster && !m_pExternalIpCheck)
		{
			m_threadsync.Lock();
			m_pExternalIpCheck = new CExternalIpCheck(this);
			m_threadsync.Unlock();
		}
	}
	else if (wParam==m_nRateTimer)
	{
		if (m_nSendCount)
		{	
			if (PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_SEND, m_nSendCount))
				m_nSendCount=0;
		}
		if (m_nRecvCount)
		{	
			if (PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_RECV, m_nRecvCount))
				m_nRecvCount=0;
		}

		if (m_bIsMaster)
		{
			EGCS;

			std::list<CServerThread *>::iterator iter;

			//Only update the speed limits from the rule set every 2 seconds to improve performance
			if (!m_nLoopCount)
			{
				m_nLastUlLimit = m_pOptions->GetCurrentSpeedLimit(1);
				m_nLastDlLimit = m_pOptions->GetCurrentSpeedLimit(0);
			}
			++m_nLoopCount %= 20;

			//Uploads first
			int nLimit = m_nLastUlLimit;
			if (nLimit == -1)
				for (iter = m_sInstanceList.begin(); iter != m_sInstanceList.end(); iter++)
				{
					CServerThread *pThread = *iter;
					pThread->m_threadsync.Lock();
					pThread->m_SlQuota.nBytesAllowedToUl = -1;
					pThread->m_SlQuota.nUploaded = 0;
					pThread->m_threadsync.Unlock();
				}
			else
			{
				nLimit *= 100;
				
				int nRemaining = nLimit;
				int nThreadLimit = nLimit / m_sInstanceList.size();
				
				std::list<CServerThread *> fullUsageList;
				
				for (iter = m_sInstanceList.begin(); iter != m_sInstanceList.end(); iter++)
				{
					CServerThread *pThread = *iter;
					pThread->m_threadsync.Lock();
					pThread->GatherTransferedBytes();
					int r = pThread->m_SlQuota.nBytesAllowedToUl - pThread->m_SlQuota.nUploaded;
					if (r>0 && pThread->m_SlQuota.nBytesAllowedToUl <= nThreadLimit)
					{
						pThread->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
						nRemaining -= pThread->m_SlQuota.nUploaded;
						pThread->m_SlQuota.nUploaded = 0;
					}
					else if (r>0 && pThread->m_SlQuota.nUploaded < nThreadLimit)
					{
						pThread->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
						nRemaining -= pThread->m_SlQuota.nUploaded;
						pThread->m_SlQuota.nUploaded = 0;
					}
					else
					{
						fullUsageList.push_back(pThread);
						pThread->m_threadsync.Unlock();
						continue;
					}
					pThread->m_threadsync.Unlock();
				}
				
				std::list<CServerThread *> fullUsageList2;
				if (!fullUsageList.empty())
				{
					nThreadLimit = nRemaining / fullUsageList.size();
					for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
					{	
						CServerThread *pThread = *iter;
						int r = pThread->m_SlQuota.nBytesAllowedToUl - pThread->m_SlQuota.nUploaded;
						if (r>0)
						{
							if (pThread->m_SlQuota.nUploaded > nThreadLimit)
								pThread->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
							else
								pThread->m_SlQuota.nBytesAllowedToUl = pThread->m_SlQuota.nUploaded;
							pThread->m_SlQuota.nUploaded = 0;
							nRemaining -= pThread->m_SlQuota.nBytesAllowedToUl;
						}
						else
						{
							fullUsageList2.push_back(pThread);
							pThread->m_threadsync.Unlock();
							continue;
						}
						pThread->m_threadsync.Unlock();
					}
					
					if (!fullUsageList2.empty())
					{
						nThreadLimit = nRemaining / fullUsageList2.size();
						for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
						{	
							CServerThread *pThread = *iter;
							pThread->m_SlQuota.nUploaded = 0;
							pThread->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
							pThread->m_threadsync.Unlock();
						}
					}
				}
			}

			//Now process the downloads
			nLimit = m_nLastDlLimit;
			if (nLimit == -1)
				for (iter = m_sInstanceList.begin(); iter != m_sInstanceList.end(); iter++)
				{
					CServerThread *pThread = *iter;
					pThread->m_threadsync.Lock();
					pThread->m_SlQuota.nBytesAllowedToDl = -1;
					pThread->m_SlQuota.nDownloaded = 0;
					pThread->m_threadsync.Unlock();
				}
			else
			{
				nLimit *= 100;
				
				int nRemaining = nLimit;
				int nThreadLimit = nLimit / m_sInstanceList.size();

				std::list<CServerThread *> fullUsageList;
				
				for (iter = m_sInstanceList.begin(); iter != m_sInstanceList.end(); iter++)
				{
					CServerThread *pThread = *iter;
					pThread->m_threadsync.Lock();
					pThread->GatherTransferedBytes();
					int r = pThread->m_SlQuota.nBytesAllowedToDl - pThread->m_SlQuota.nDownloaded;
					if (r>0 && pThread->m_SlQuota.nBytesAllowedToDl <= nThreadLimit)
					{
						pThread->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
						nRemaining -= pThread->m_SlQuota.nDownloaded;
						pThread->m_SlQuota.nDownloaded = 0;
					}
					else if (r>0 && pThread->m_SlQuota.nDownloaded < nThreadLimit)
					{
						pThread->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
						nRemaining -= pThread->m_SlQuota.nDownloaded;
						pThread->m_SlQuota.nDownloaded = 0;
					}
					else
					{
						fullUsageList.push_back(pThread);
						//pThread->m_threadsync.Unlock();
						continue;
					}
					pThread->m_threadsync.Unlock();
				}

				std::list<CServerThread *> fullUsageList2;				
				if (!fullUsageList.empty())
				{
					nThreadLimit = nRemaining / fullUsageList.size();
					for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
					{	
						CServerThread *pThread = *iter;
						int r = pThread->m_SlQuota.nBytesAllowedToDl - pThread->m_SlQuota.nDownloaded;
						if (r>0)
						{
							if (pThread->m_SlQuota.nDownloaded > nThreadLimit)
								pThread->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
							else
								pThread->m_SlQuota.nBytesAllowedToDl = pThread->m_SlQuota.nDownloaded;
							pThread->m_SlQuota.nDownloaded = 0;
							nRemaining -= pThread->m_SlQuota.nBytesAllowedToDl;
						}
						else
						{
							fullUsageList2.push_back(pThread);
							//pThread->m_threadsync.Unlock();
							continue;
						}
						pThread->m_threadsync.Unlock();
					}
					
					if (!fullUsageList2.empty())
					{
						nThreadLimit = nRemaining / fullUsageList2.size();
						for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
						{	
							CServerThread *pThread = *iter;
							pThread->m_SlQuota.nDownloaded = 0;
							pThread->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
							pThread->m_threadsync.Unlock();
						}
					}
				}
			}
			
			LGCS;
		}
		ProcessNewSlQuota();
	}
	else if (m_pExternalIpCheck && wParam == m_pExternalIpCheck->m_nTimerID)
	{
		m_threadsync.Lock();
		m_pExternalIpCheck->OnTimer();
		m_threadsync.Unlock();
	}
}

const int CServerThread::GetGlobalNumConnections()
{
	EGCS;
	int num=m_userids.size();
	LGCS;
	return num;
}

CControlSocket * CServerThread::GetControlSocket(int userid)
{
	CControlSocket *ret=0;
	m_threadsync.Lock();
	std::map<int, CControlSocket *>::iterator iter=m_LocalUserIDs.find(userid);
	if (iter!=m_LocalUserIDs.end())
		ret=iter->second;
	m_threadsync.Unlock();
	return ret;
}

void CServerThread::ProcessControlMessage(t_controlmessage *msg)
{
	if (msg->command==USERCONTROL_KICK)
	{
		CControlSocket *socket=GetControlSocket(msg->socketid);
		if (socket)
			socket->ForceClose(4);
	}
	delete msg;
}

BOOL CServerThread::IsReady()
{
	return !m_bQuit;
}

int CServerThread::GetIpCount(const CStdString &ip) const
{
	int count=0;
	EGCS;
	std::map<CStdString, int>::iterator iter=m_userIPs.find(ip);
	if (iter!=m_userIPs.end())
		count=iter->second;
	LGCS;
	return count;
}

void CServerThread::IncIpCount(const CStdString &ip)
{
	int count;
	EGCS;
	std::map<CStdString, int>::iterator iter=m_userIPs.find(ip);
	if (iter!=m_userIPs.end())
		count=iter->second++;
	else
		m_userIPs[ip]=1;
	LGCS;
}

void CServerThread::DecIpCount(const CStdString &ip)
{
	EGCS;
	std::map<CStdString, int>::iterator iter=m_userIPs.find(ip);
	ASSERT(iter!=m_userIPs.end());
	if (iter==m_userIPs.end())
	{
		LGCS;
		return;
	}
	else
	{
		ASSERT(iter->second);
		if (iter->second)
			iter->second--;
	}
	LGCS;
}

void CServerThread::IncSendCount(int count)
{
	m_nSendCount += count;
}

void CServerThread::IncRecvCount(int count)
{
	m_nRecvCount += count;
}

void CServerThread::DontQuit()
{
	m_threadsync.Lock();
	m_bQuit = FALSE;
	m_threadsync.Unlock();
}

void CServerThread::ProcessNewSlQuota()
{
	m_threadsync.Lock();

	std::map<int, CControlSocket *>::iterator iter;

	//Uploads first	
	if (m_SlQuota.nBytesAllowedToUl == -1)
	{
		for (iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
		{
			CControlSocket *pControlSocket = iter->second;
			BOOL bContinue = FALSE;
			if (pControlSocket->m_SlQuota.nBytesAllowedToUl > 0 && !(pControlSocket->m_SlQuota.nBytesAllowedToUl - pControlSocket->m_SlQuota.nUploaded))
				bContinue = TRUE;
			pControlSocket->m_SlQuota.nBytesAllowedToUl = -1;
			pControlSocket->m_SlQuota.nUploaded = 0;
		}
	}
	else
	{
		int nRemaining = m_SlQuota.nBytesAllowedToUl;
		int nThreadLimit = nRemaining / m_sInstanceList.size();
			
		std::list<CControlSocket *> fullUsageList;
		
		for (iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
		{
			CControlSocket *pControlSocket = iter->second;
			int r = pControlSocket->m_SlQuota.nBytesAllowedToUl - pControlSocket->m_SlQuota.nUploaded;
			if (r && pControlSocket->m_SlQuota.nBytesAllowedToUl <= nThreadLimit)
			{
				pControlSocket->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
				nRemaining -= pControlSocket->m_SlQuota.nUploaded;
				pControlSocket->m_SlQuota.nUploaded = 0;
			}
			else if (r && pControlSocket->m_SlQuota.nUploaded < nThreadLimit)
			{
				pControlSocket->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
				nRemaining -= pControlSocket->m_SlQuota.nUploaded;
				pControlSocket->m_SlQuota.nUploaded = 0;
			}
			else
			{
				fullUsageList.push_back(pControlSocket);
				continue;
			}
		}
		
		std::list<CControlSocket *> fullUsageList2;
		if (!fullUsageList.empty())
		{
			std::list<CControlSocket *>::iterator iter;
			
			nThreadLimit = nRemaining / fullUsageList.size();
			for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
			{	
				CControlSocket *pControlSocket = *iter;
				int r = pControlSocket->m_SlQuota.nBytesAllowedToUl - pControlSocket->m_SlQuota.nUploaded;
				if (r)
				{
					if (pControlSocket->m_SlQuota.nUploaded > nThreadLimit)
						pControlSocket->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
					else
						pControlSocket->m_SlQuota.nBytesAllowedToUl = pControlSocket->m_SlQuota.nUploaded;
					pControlSocket->m_SlQuota.nUploaded = 0;
					nRemaining -= pControlSocket->m_SlQuota.nBytesAllowedToUl;
				}
				else
				{
					fullUsageList2.push_back(pControlSocket);
					continue;
				}
			}
			
			if (!fullUsageList2.empty())
			{
				nThreadLimit = nRemaining / fullUsageList2.size();
				for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
				{	
					CControlSocket *pControlSocket = *iter;
					pControlSocket->m_SlQuota.nUploaded = 0;
					pControlSocket->m_SlQuota.nBytesAllowedToUl = nThreadLimit;
				}
			}
		}
	}

	//Now process download limits
	if (m_SlQuota.nBytesAllowedToDl == -1)
	{
		for (iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
		{
			CControlSocket *pControlSocket = iter->second;
			BOOL bContinue = FALSE;
			if (pControlSocket->m_SlQuota.nBytesAllowedToDl > 0 && !(pControlSocket->m_SlQuota.nBytesAllowedToDl - pControlSocket->m_SlQuota.nDownloaded))
				bContinue = TRUE;
			pControlSocket->m_SlQuota.nBytesAllowedToDl = -1;
			pControlSocket->m_SlQuota.nDownloaded = 0;
		}
	}
	else
	{
		int nRemaining = m_SlQuota.nBytesAllowedToDl;
		int nThreadLimit = nRemaining / m_sInstanceList.size();
		
		std::list<CControlSocket *> fullUsageList;
		for (iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
		{
			CControlSocket *pControlSocket = iter->second;
			int r = pControlSocket->m_SlQuota.nBytesAllowedToDl - pControlSocket->m_SlQuota.nDownloaded;
			if (r && pControlSocket->m_SlQuota.nBytesAllowedToDl <= nThreadLimit)
			{
				pControlSocket->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
				nRemaining -= pControlSocket->m_SlQuota.nDownloaded;
				pControlSocket->m_SlQuota.nDownloaded = 0;
			}
			else if (r && pControlSocket->m_SlQuota.nDownloaded < nThreadLimit)
			{
				pControlSocket->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
				nRemaining -= pControlSocket->m_SlQuota.nDownloaded;
				pControlSocket->m_SlQuota.nDownloaded = 0;
			}
			else
			{
				fullUsageList.push_back(pControlSocket);
				continue;
			}
		}
		
		std::list<CControlSocket *> fullUsageList2;
		if (!fullUsageList.empty())
		{
			std::list<CControlSocket *>::iterator iter;
			
			nThreadLimit = nRemaining / fullUsageList.size();
			for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
			{	
				CControlSocket *pControlSocket = *iter;
				int r = pControlSocket->m_SlQuota.nBytesAllowedToDl - pControlSocket->m_SlQuota.nDownloaded;
				if (r)
				{
					if (pControlSocket->m_SlQuota.nDownloaded > nThreadLimit)
						pControlSocket->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
					else
						pControlSocket->m_SlQuota.nBytesAllowedToDl = pControlSocket->m_SlQuota.nDownloaded;
					pControlSocket->m_SlQuota.nDownloaded = 0;
					nRemaining -= pControlSocket->m_SlQuota.nBytesAllowedToDl;
				}
				else
				{
					fullUsageList2.push_back(pControlSocket);
					continue;
				}
			}
			
			if (!fullUsageList2.empty())
			{
				nThreadLimit = nRemaining / fullUsageList2.size();
				for (iter = fullUsageList.begin(); iter != fullUsageList.end(); iter++)
				{	
					CControlSocket *pControlSocket = *iter;
					pControlSocket->m_SlQuota.nDownloaded = 0;
					pControlSocket->m_SlQuota.nBytesAllowedToDl = nThreadLimit;
				}
			}
		}
	}

	for (iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
	{
		CControlSocket *pControlSocket = iter->second;
		pControlSocket->Continue();
	}

	m_threadsync.Unlock();
}

void CServerThread::GatherTransferedBytes()
{
	m_threadsync.Lock();
	for (std::map<int, CControlSocket *>::iterator iter = m_LocalUserIDs.begin(); iter != m_LocalUserIDs.end(); iter++)
	{
		if (iter->second->m_SlQuota.nBytesAllowedToDl != -1)
			if (iter->second->m_SlQuota.bDownloadBypassed)
				iter->second->m_SlQuota.nDownloaded = 0;
			else
				m_SlQuota.nDownloaded += iter->second->m_SlQuota.nDownloaded;
		if (iter->second->m_SlQuota.nBytesAllowedToUl != -1)
			if (iter->second->m_SlQuota.bUploadBypassed)
				iter->second->m_SlQuota.nUploaded = 0;
			else
				m_SlQuota.nUploaded += iter->second->m_SlQuota.nUploaded;

		 iter->second->m_SlQuota.bDownloadBypassed = iter->second->m_SlQuota.bUploadBypassed = FALSE;
	}
	m_threadsync.Unlock();
}

CStdString CServerThread::GetExternalIP()
{
	CStdString ip;
	m_threadsync.Lock();
	if (m_pExternalIpCheck)
		ip = m_pExternalIpCheck->GetIP();
	else
	{
		EGCS;
		CServerThread *pThread = m_sInstanceList.front();
		pThread->m_threadsync.Lock();
		if (pThread != this && pThread->m_pExternalIpCheck)
			ip = pThread->m_pExternalIpCheck->GetIP();
		pThread->m_threadsync.Unlock();
		LGCS;
	}
	m_threadsync.Unlock();

	return ip;
}

void CServerThread::ExternalIPFailed()
{
	CStdString ip;
	m_threadsync.Lock();
	if (m_pExternalIpCheck)
		m_pExternalIpCheck->TriggerUpdate();
	else
	{
		EGCS;
		CServerThread *pThread = m_sInstanceList.front();
		if (pThread != this && pThread->m_pExternalIpCheck)
			pThread->m_pExternalIpCheck->TriggerUpdate();
		LGCS;
	}
	m_threadsync.Unlock();
}