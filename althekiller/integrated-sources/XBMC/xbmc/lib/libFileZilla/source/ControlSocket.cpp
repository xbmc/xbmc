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

// ControlSocket.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "ControlSocket.h"
#include "transfersocket.h"
#include "ServerThread.h"
#include "Options.h"
#include "Permissions.h"
#ifndef NOLAYERS
#include "AsyncGssSocketLayer.h"
#endif

#if defined(_XBOX)
#include "utils/Log.h"
#include "xbox/IoSupport.h"
#include "Util.h"
#include "Utils/log.h"
#include "GUISettings.h"
#include "ApplicationMessenger.h"
#include "utils/MemoryUnitManager.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CControlSocket

std::map<CStdString, int> CControlSocket::m_UserCount;
CCriticalSectionWrapper CControlSocket::m_Sync;

CControlSocket::CControlSocket(CServerThread *pOwner)
{
	m_status.loggedon=FALSE;
	m_transferstatus.socket = NULL;
	m_transferstatus.ip="";
	m_transferstatus.port=-1;
	m_transferstatus.pasv=-1;
	m_transferstatus.rest=0;
	m_transferstatus.type=-1;
	m_bWaitGoOffline=FALSE;
	GetSystemTime(&m_LastTransferTime);
	GetSystemTime(&m_LastCmdTime);
	GetSystemTime(&m_LoginTime);
	m_bQuitCommand = FALSE;
	
	ASSERT(pOwner);
	m_pOwner=pOwner;
	
	m_nTelnetSkip = 0;
	m_nRecvBufferPos = 0;

	m_pSendBuffer = NULL;
	m_nSendBufferLen = 0;

	m_pGssLayer = NULL;

	memset(&m_SlQuota, 0, sizeof(t_Quota));
	m_SlQuota.bContinueUpload = m_SlQuota.bContinueDownload = FALSE;

	m_SlQuota.nBytesAllowedToDl = -1;
	m_SlQuota.nBytesAllowedToUl = -1;
	m_SlQuota.bDownloadBypassed = TRUE;
	m_SlQuota.bUploadBypassed = TRUE;
}

CControlSocket::~CControlSocket()
{
	if (m_status.loggedon)
	{
		DecUserCount(m_status.user);
		m_pOwner->DecIpCount(m_status.ip);
		m_status.loggedon=FALSE;
	}
	t_connectiondata *conndata=new t_connectiondata;
	t_connop *op = new t_connop;
  memset(conndata, 0, sizeof(t_connectiondata));
  memset(op, 0, sizeof(t_connop));

	op->data = conndata;
	op->op = USERCONTROL_CONNOP_REMOVE;
	conndata->userid = m_userid;
	if (!PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_CONNECTIONDATA, (LPARAM)op))
	{
		delete op->data;
		delete op;
	}
	if (m_transferstatus.socket)
		delete m_transferstatus.socket;
	m_transferstatus.socket=0;

	delete [] m_pSendBuffer;
	m_nSendBufferLen = 0;
#ifndef NOLAYERS
	RemoveAllLayers();
	delete m_pGssLayer;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Member-Funktion CControlSocket 

#define BUFFERSIZE 500
void CControlSocket::OnReceive(int nErrorCode) 
{
	int len = BUFFERSIZE;
	int nLimit = GetSpeedLimit(1);
	if (!nLimit)
		return;
	if (len > nLimit && nLimit != -1)
		len = nLimit;
	
	unsigned char *buffer = new unsigned char[BUFFERSIZE];
	int numread = Receive(buffer, len);
	if (numread!=SOCKET_ERROR && numread)
	{
		if (nLimit != -1)
			m_SlQuota.nUploaded += numread;

		m_pOwner->IncRecvCount(numread);
		//Parse all received bytes
		for (int i=0; i<numread; i++)
		{
			if (!m_nRecvBufferPos)
			{
				//Remove telnet characters
				if (m_nTelnetSkip)
				{
					if (m_nTelnetSkip == 2)
					{
						if (buffer[i]<251 || buffer[i]>254)
							m_nTelnetSkip--;
					}
					m_nTelnetSkip--;
				}
				else if (buffer[i] == 255)
					m_nTelnetSkip = 2;

				if (m_nTelnetSkip)
					continue;
			}

			//Check for line endings
			if ((buffer[i]=='\r')||(buffer[i]==0)||(buffer[i]=='\n'))
			{
				//If input buffer is not empty...
				if (m_nRecvBufferPos)
				{
					m_RecvBuffer[m_nRecvBufferPos] = 0;
					m_RecvLineBuffer.push_back(m_RecvBuffer);
					m_nRecvBufferPos = 0;

					//Signal that there is a new command waiting to be processed.
					GetSystemTime(&m_LastCmdTime);
					m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_COMMAND, m_userid);
				}
			}
			else
				//The command may only be 2000 chars long. This ensures that a malicious user can't
				//send extremely large commands to fill the memory of the server
				if (m_nRecvBufferPos < 2000)
					m_RecvBuffer[m_nRecvBufferPos++] = buffer[i];
		}
	}
	else
	{
		if (!numread || GetLastError()!=WSAEWOULDBLOCK)
		{
			//Control connection has been closed
			Close();
			SendStatus("disconnected.",0);
			m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_DELSOCKET, m_userid);
		}
	}

	delete [] buffer;
}

BOOL CControlSocket::GetCommand(CStdString &command, CStdString &args)
{
	//Get first command from input buffer
	CStdString str;
	if (m_RecvLineBuffer.empty())
		return FALSE;
	str = m_RecvLineBuffer.front();
	m_RecvLineBuffer.pop_front();

	//Output command in status window
	CStdString str2=str;
	str2.MakeUpper();

	//Hide passwords if the server admin wants to.
	if (str2.Left(5)=="PASS ")
	{	if (m_pOwner->m_pOptions->GetOptionVal(OPTION_LOGSHOWPASS))
			SendStatus(str,2);
		else
		{
			str2=str.Left(5);
			for (int i=0;i<str.GetLength()-5;i++)
				str2+="*";
			SendStatus(str2,2);
		}
	}
	else
		SendStatus(str,2);
	
	//Split command and arguments
	int pos = str.Find(" ");
	if (pos!=-1)
	{
		command = str.Left(pos);
		args = str.Mid(pos+1);
		args.TrimLeft(" ");
		args.TrimRight(" ");
	}
	else
		command = str;
	if (command == "")
		return FALSE;
	command.MakeUpper();
	return TRUE;
}


#if defined(_XBOX)
BOOL CControlSocket::GetCommandFromString(const CStdString& source, CStdString &command,CStdString &args)
{
	//Get first command from input buffer
	if (source.empty())
		return FALSE;

	//Output command in status window
	CStdString str2=source;
	str2.MakeUpper();

  //SendStatus(source,2); // already done in GetCommand()
	
	//Split command and arguments
	int pos=source.Find(" ");
	if (pos!=-1)
	{
		command=source.Left(pos);
		args=source.Mid(pos+1);
		args.TrimLeft(" ");
		args.TrimRight(" ");
	}
	else
		command=source;
	if (command=="")
		return FALSE;
	//command.MakeUpper(); // oh no we dont, breaks e.g. site playmedia(http://foo)
	return TRUE;
}
#endif

void CControlSocket::SendStatus(LPCTSTR status, int type)
{
	t_statusmsg *msg=new t_statusmsg;
	strcpy(msg->ip, m_RemoteIP);
	GetSystemTime(&msg->time);
	if (!m_status.loggedon)
	{
		msg->user = new char[16];
		strcpy(msg->user, "(not logged in)");
	}
	else
	{
		msg->user = new char[strlen(m_status.user)+1];
		strcpy(msg->user, m_status.user);
	}
	msg->userid=m_userid;
	msg->type=type;
	msg->status = new char[strlen(status)+1];
	strcpy(msg->status, status);
	if (!PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_STATUSMESSAGE, (LPARAM)msg))
	{
		delete [] msg->status;
		delete [] msg->user;
		delete msg;
	}
}

#if defined(_XBOX)
BOOL CControlSocket::SendCurDir(const CStdString command,CStdString curDir)
{
  return SendDir(command, curDir, " is current directory.");
}

BOOL CControlSocket::SendDir(const CStdString command,CStdString curDir,const CStdString prompt)
{
  if (1 /*g_stSettings.m_bFTPSingleCharDrives*/
    && (curDir.GetLength() >= 2) 
    && (curDir[0] == '/') && isalpha(curDir[1]) && ((curDir.GetLength() == 2) || (curDir[2] == ':')))
  {
    // modified to be consistent with other xbox ftp behavior: drive 
    // name is a single character without the ':' at the end
    if (curDir.GetLength() > 3) 
      curDir = curDir.Left(2).ToUpper() + curDir.Mid(3);
    else
      curDir = curDir.Left(2).ToUpper();
  }

  CStdString str;
  str.Format("%s \"%s\"%s", command, curDir, prompt);
	return Send(str);
}
#endif

BOOL CControlSocket::Send(LPCTSTR str)
{
	char *buffer = new char[strlen(str) + 3];
	strcpy(buffer, str);
	strcat(buffer, "\r\n");
	int len = strlen(buffer);
	
	//Add line to back of send buffer if it's not empty
	if (m_pSendBuffer)
	{
		char *tmp = m_pSendBuffer;
		m_pSendBuffer = new char[m_nSendBufferLen + len];
		memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
		memcpy(m_pSendBuffer+m_nSendBufferLen, buffer, len);
		delete [] tmp;
		m_nSendBufferLen += len;
		delete [] buffer;
		return TRUE;
	}

	int nLimit = GetSpeedLimit(0);
	if (!nLimit)
	{
		if (!m_pSendBuffer)
		{
			m_pSendBuffer = new char[len];
			memcpy(m_pSendBuffer, buffer, len);
			m_nSendBufferLen = len;
		}
		else
		{
			char *tmp = m_pSendBuffer;
			m_pSendBuffer = new char[m_nSendBufferLen + len];
			memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
			memcpy(m_pSendBuffer+m_nSendBufferLen, buffer, len);
			delete [] tmp;
			m_nSendBufferLen += len;
		}
		delete [] buffer;
		return TRUE;
	}
	int numsend = nLimit;
	if (numsend == -1 || len < numsend)
		numsend = len;

	int res=CAsyncSocketEx::Send(buffer, numsend);
	if (res==SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
	{
		res = 0;
	}
	else if (!res || res==SOCKET_ERROR)
	{
		delete [] buffer;
		Close();
		SendStatus("could not send reply, disconnected.",0);	
		m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_DELSOCKET, m_userid);
		return FALSE;
	}
	
	if (nLimit != -1)
		m_SlQuota.nDownloaded += res;

	if (res != len)
	{
		if (!m_pSendBuffer)
		{
			m_pSendBuffer = new char[len-res];
			memcpy(m_pSendBuffer, buffer+res, len-res);
			m_nSendBufferLen = len-res;
		}
		else
		{
			char *tmp = m_pSendBuffer;
			m_pSendBuffer = new char[m_nSendBufferLen + len - res];
			memcpy(m_pSendBuffer, tmp, m_nSendBufferLen);
			memcpy(m_pSendBuffer+m_nSendBufferLen, buffer+res, len-res);
			delete [] tmp;
			m_nSendBufferLen += len-res;
		}
		TriggerEvent(FD_WRITE);
	}
	delete [] buffer;
	SendStatus(str, 3);
	m_pOwner->IncSendCount(res);
	return TRUE;
}

void CControlSocket::OnClose(int nErrorCode) 
{
	Close();
	SendStatus("disconnected.",0);	
	m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_DELSOCKET, m_userid);
	CAsyncSocketEx::OnClose(nErrorCode);
}

#define COMMAND_USER	0
#define COMMAND_PASS	1
#define COMMAND_QUIT	2
#define COMMAND_CWD		3
#define COMMAND_PWD		4
#define COMMAND_PORT	5
#define COMMAND_PASV	6
#define COMMAND_TYPE	7
#define COMMAND_LIST	8
#define COMMAND_REST	9
#define COMMAND_CDUP	10
#define COMMAND_RETR	11
#define COMMAND_STOR	12
#define COMMAND_SIZE	13
#define COMMAND_DELE	14
#define COMMAND_RMD		15
#define COMMAND_MKD		16
#define COMMAND_RNFR	17
#define COMMAND_RNTO	18
#define COMMAND_ABOR	19
#define COMMAND_SYST	20
#define COMMAND_NOOP	21
#define COMMAND_APPE	22
#define COMMAND_NLST	23
#define COMMAND_MDTM	24
#define COMMAND_XPWD	25
#define COMMAND_XCUP	26
#define COMMAND_XMKD	27
#define COMMAND_XRMD	28
#define COMMAND_NOP		29
#define COMMAND_EPSV	30
#define COMMAND_EPRT	31
#define COMMAND_AUTH	32
#define COMMAND_ADAT	33
#define COMMAND_PBSZ	34
#define COMMAND_PROT	35


#if defined(_XBOX)
#define COMMAND_SITE    1032

// SITE commands
#define SITE_CRC      1
#define SITE_EXECUTE  2
//#define SITE_REBOOT   3
//#define SITE_SHUTDOWN 4
#endif

typedef struct
{
	int nID;
#if defined(_XBOX)
	char command[9];
#else
	char command[5];
#endif
	BOOL bHasargs;
	BOOL bValidBeforeLogon;
} t_command;

static const t_command commands[]={	COMMAND_USER, "USER", TRUE,	 TRUE,
									COMMAND_PASS, "PASS", FALSE, TRUE,
									COMMAND_QUIT, "QUIT", FALSE, TRUE,
									COMMAND_CWD,  "CWD",  TRUE,  FALSE,
									COMMAND_PWD,  "PWD",  FALSE, FALSE,
									COMMAND_PORT, "PORT", TRUE,  FALSE,
									COMMAND_PASV, "PASV", FALSE, FALSE,
									COMMAND_TYPE, "TYPE", TRUE,  FALSE,
									COMMAND_LIST, "LIST", FALSE, FALSE,
									COMMAND_REST, "REST", TRUE,  FALSE,
									COMMAND_CDUP, "CDUP", FALSE, FALSE,
									COMMAND_RETR, "RETR", TRUE,  FALSE,
									COMMAND_STOR, "STOR", TRUE,  FALSE,
									COMMAND_SIZE, "SIZE", TRUE,  FALSE,
									COMMAND_DELE, "DELE", TRUE,  FALSE,
									COMMAND_RMD,  "RMD",  TRUE,  FALSE,
									COMMAND_MKD,  "MKD",  TRUE,  FALSE,
									COMMAND_RNFR, "RNFR", TRUE,  FALSE,
									COMMAND_RNTO, "RNTO", TRUE,  FALSE,
									COMMAND_ABOR, "ABOR", FALSE, FALSE,
									COMMAND_SYST, "SYST", FALSE, TRUE,
									COMMAND_NOOP, "NOOP", FALSE, FALSE,
									COMMAND_APPE, "APPE", TRUE,  FALSE,
									COMMAND_NLST, "NLST", FALSE, FALSE,
									COMMAND_MDTM, "MDTM", TRUE,  FALSE,
									COMMAND_XPWD, "XPWD", FALSE, FALSE,
									COMMAND_XCUP, "XCUP", FALSE, FALSE,
									COMMAND_XMKD, "XMKD", TRUE,  FALSE,
									COMMAND_XRMD, "XRMD", TRUE,  FALSE,
									COMMAND_NOP,  "NOP",  FALSE, FALSE,
									COMMAND_EPSV, "EPSV", FALSE, FALSE,
									COMMAND_EPRT, "EPRT", TRUE,  FALSE,
									COMMAND_AUTH, "AUTH", TRUE,  TRUE,
									COMMAND_ADAT, "ADAT", TRUE,  TRUE,
									COMMAND_PBSZ, "PBSZ", TRUE,  TRUE, 
									COMMAND_PROT, "PROT", TRUE,  TRUE

                #if defined(_XBOX)
                  ,
                  COMMAND_SITE,     "SITE", TRUE, FALSE
                #endif

						};

#if defined(_XBOX)
static const t_command site_commands[]={
    SITE_CRC,      "CRC", TRUE, FALSE,
    SITE_EXECUTE,  "EXECUTE", TRUE, FALSE //,
    //SITE_REBOOT,   "REBOOT", FALSE, FALSE,
    //SITE_SHUTDOWN, "SHUTDOWN", FALSE, FALSE
};
#endif

void CControlSocket::ParseCommand()
{
	//Get command
	CStdString command, args;
	if (!GetCommand(command, args))
		return;

	//Check if command is valid
	int nCommandID = -1;
	for (int i=0;i<(sizeof(commands)/sizeof(t_command));i++)
	{
		if (command == commands[i].command)
		{
			//Does the command needs an argument?
			if (commands[i].bHasargs && (args==""))
			{
				Send("501 Syntax error");
				return;
			}
			//Can it be issued before logon?
			else if (!m_status.loggedon && !commands[i].bValidBeforeLogon)
			{
				Send("530 Please log in with USER and PASS first.");
				return;
			}
			nCommandID = commands[i].nID;
			break;			
		}
	}
	//Command not recognized
	if (nCommandID == -1)
	{
		Send("500 Syntax error, command unrecognized.");
		return;
	}

	//Now process the commands
	switch (nCommandID)
	{
	case COMMAND_USER:
		{
			if (m_status.loggedon)
			{
				GetSystemTime(&m_LoginTime);
				DecUserCount(m_status.user);
				m_pOwner->DecIpCount(m_status.ip);
				t_connectiondata *conndata = new t_connectiondata;
				t_connop *op = new t_connop;

        memset(conndata, 0, sizeof(t_connectiondata));
        memset(op, 0, sizeof(t_connop));

				op->data = conndata;
				op->op = USERCONTROL_CONNOP_MODIFY;
				conndata->userid = m_userid;
				conndata->pThread = m_pOwner;
				
				SOCKADDR_IN sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				int nSockAddrLen = sizeof(sockAddr);
				BOOL bResult = GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
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
			}
			m_status.loggedon = FALSE;
			RenName = "";
			args.MakeLower();
			m_status.user=args;
#ifndef NOLAYERS
			if (m_pGssLayer && m_pGssLayer->AuthSuccessful())
			{	
				char sendme[4096];

				int res = m_pGssLayer->ProcessCommand("USER", args, sendme);
				if (res != -1)
					DoUserLogin(sendme);
				Send(sendme);
			}
			else
#endif
				Send("331 Password required for "+args);
		}
		break;
	case COMMAND_PASS:
		if (m_status.loggedon)
			Send("503 Bad sequence of commands.");
#ifndef NOLAYERS
		else if (m_pGssLayer && m_pGssLayer->AuthSuccessful())
		{
			char sendme[4096];
			int res = m_pGssLayer->ProcessCommand("PASS", m_status.user, args, sendme);
			if (res != -1)
				DoUserLogin(sendme);
			Send(sendme);
		}
#endif
		else
		{
			CUser user;
			if (m_pOwner->m_pPermissions->Lookup(m_status.user, args, user))
			{
				if (!user.BypassUserLimit())
				{
					int nMaxUsers = (int)m_pOwner->m_pOptions->GetOptionVal(OPTION_MAXUSERS);
					if (m_pOwner->GetGlobalNumConnections()>nMaxUsers&&nMaxUsers)
					{
						SendStatus("Refusing connection. Reason: Max. connection count reached.",1);
						Send("421 Too many users are connected, please try again later.");
						ForceClose(-1);
						break;
					}
				}
		
				else if (user.GetUserLimit() && GetUserCount(m_status.user)>=user.GetUserLimit())
				{
					CStdString str;
					str.Format("Refusing connection. Reason: Max. connection count reached for the user \"%s\".",m_status.user);
					SendStatus(str,1);
					Send("421 Too many users logged in for this account. Try again later.");
					ForceClose(-1);
					break;
				}

				CStdString ip;
				unsigned int nPort;
				
				SOCKADDR_IN sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				int nSockAddrLen = sizeof(sockAddr);
				BOOL bResult = GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
				if (bResult)
				{
					nPort = ntohs(sockAddr.sin_port);
					ip = inet_ntoa(sockAddr.sin_addr);
				}
				
				int count=m_pOwner->GetIpCount(ip);
				if (user.nIpLimit && count>=user.GetIpLimit())
				{
					CStdString str;
					if (count==1)
						str.Format("Refusing connection. Reason: No more connections allowed from your IP. (%s already connected once)",ip);
					else
						str.Format("Refusing connection. Reason: No more connections allowed from your IP. (%s already connected %d times)", ip, count);
					SendStatus(str,1);
					Send("421 Refusing connection. No more connections allowed from your IP.");
					ForceClose(-1);
					break;
				}
	
				m_CurrentDir = m_pOwner->m_pPermissions->GetHomeDir(m_status.user);
				if (m_CurrentDir=="")
				{
					Send("550 Could not get home dir!");
					ForceClose(-1);
					break;
				}
				
				m_status.ip = ip;

				count = GetUserCount(user.user);
				if (user.GetUserLimit() && count>=user.GetUserLimit())
				{
					CStdString str;
					str.Format("Refusing connection. Reason: Maximum connection count (%d) reached for this user", user.GetUserLimit());
					SendStatus(str,1);
					str.Format("421 Refusing connection. Maximum connection count reached for the user '%s'", user.user);
					Send(str);
					ForceClose(-1);
					break;
				}

				m_pOwner->IncIpCount(ip);
				IncUserCount(m_status.user);
				m_status.loggedon=TRUE;
				Send("230 Logged on");
				GetSystemTime(&m_LastTransferTime);
				m_pOwner->m_pPermissions->AutoCreateDirs(m_status.user);

				t_connectiondata *conndata = new t_connectiondata;
				t_connop *op = new t_connop;

        memset(conndata, 0, sizeof(t_connectiondata));
        memset(op, 0, sizeof(t_connop));

				op->data = conndata;
				op->op = USERCONTROL_CONNOP_MODIFY;
				conndata->userid = m_userid;
				conndata->user = m_status.user;
				conndata->pThread = m_pOwner;

				memset(&sockAddr, 0, sizeof(sockAddr));
				nSockAddrLen = sizeof(sockAddr);
				bResult = GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
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
			}
			else 
				Send("530 Login or password incorrect!");
		}
		break;
	case COMMAND_QUIT:
		if (m_transferstatus.socket)
			m_bQuitCommand = TRUE;
		else
			ForceClose(5);
		break;	
	case COMMAND_CWD:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => cwd to "/"  && user's homedir == "/" just set the currentdir to "/"
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((args == "/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")){
				m_CurrentDir="/";
				SendCurDir("250 CWD successful.",m_CurrentDir);
			} 
			else
			{ // check for real permissions
				
				int res = m_pOwner->m_pPermissions->ChangeCurrentDir(m_status.user,m_CurrentDir,args);
				if (!res)
				{
					SendCurDir("250 CWD successful.",m_CurrentDir);
				}
				else if (res & 1)
				{
					SendDir("550 CWD failed.", args, ": Permission denied.");
				}
				else if (res)
				{
					SendDir("550 CWD failed.", args, ": directory not found.");
				}
			}
#else
			int res = m_pOwner->m_pPermissions->ChangeCurrentDir(m_status.user,m_CurrentDir,args);
			if (!res)
			{
				CStdString str;
				str.Format("250 CWD successful. \"%s\" is current directory.",m_CurrentDir);
				Send(str);
			}
			else if (res & 1)
			{
				CStdString str;
				str.Format("550 CWD failed. \"%s\": Permission denied.",args);
				Send(str);
			}
			else if (res)
			{
				CStdString str;
				str.Format("550 CWD failed. \"%s\": directory not found.",args);
				Send(str);
			}
#endif
		}
		break;
	case COMMAND_PWD:
	case COMMAND_XPWD:
		{
			SendCurDir("257",m_CurrentDir);
		}
		break;
	case COMMAND_PORT:
		{
			if (m_transferstatus.socket)
			{
				delete m_transferstatus.socket;
				m_transferstatus.socket=0;
			}
			int count=0;
			int pos=0;
			//Convert commas to dots
			args.Replace(",",".");
			while(1)
			{
				pos=args.Find(".",pos);
				if (pos!=-1)
					count++;
				else
					break;
				pos++;
			}
			if (count!=5)
			{
				Send("501 Syntax error");
				break;
			}
			CStdString ip;
			int port = 0;
			int i=args.ReverseFind('.');
			port=atoi(args.Right(args.GetLength()-(i+1))); //get ls byte of server socket
			args=args.Left(i);
			i=args.ReverseFind('.');
			port+=256*atoi(args.Right(args.GetLength()-(i+1))); // add ms byte to server socket
			ip=args.Left(i);

			if (inet_addr(ip)==INADDR_NONE || port<1 || port>65535)
			{
				Send("501 Syntax error");
				break;
			}

			m_transferstatus.ip = ip;
			m_transferstatus.port = port;
			m_transferstatus.pasv=0;
			Send("200 Port command successful");
			break;
		}
	case COMMAND_PASV:
		{
			if (m_transferstatus.socket)
				delete m_transferstatus.socket;
			m_transferstatus.socket=new CTransferSocket(this);

			unsigned int port = 0;
			CStdString ip;
			unsigned int retries = 3;
			if (m_pOwner->m_pOptions->GetOptionVal(OPTION_CUSTOMPASVIPTYPE))
				ip = m_pOwner->GetExternalIP();

			if (ip == "")
			{
				//Get the ip of the control socket
				SOCKADDR_IN sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				
				int nSockAddrLen = sizeof(sockAddr);
				BOOL bResult = GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
				if (bResult)
					ip = inet_ntoa(sockAddr.sin_addr);

			}

			while (retries > 0)
			{
				if (m_pOwner->m_pOptions->GetOptionVal(OPTION_USECUSTOMPASVPORT))
				{
					static UINT customPort = 0;
					unsigned int minPort = (unsigned int)m_pOwner->m_pOptions->GetOptionVal(OPTION_CUSTOMPASVMINPORT);
					unsigned int maxPort = (unsigned int)m_pOwner->m_pOptions->GetOptionVal(OPTION_CUSTOMPASVMAXPORT);
					if (minPort > maxPort) {
						unsigned int temp = minPort;
						minPort = maxPort;
						maxPort = temp;
					}
					if (customPort < minPort || customPort > maxPort) {
						customPort = minPort;
					}
					port = customPort;

					++customPort;
				} else {
					port = 0;
				}
				if (m_transferstatus.socket->Create(port, SOCK_STREAM, FD_ACCEPT))
					break;
				--retries;
			}
			if (retries <= 0) {
				delete m_transferstatus.socket;
				m_transferstatus.socket = NULL;
				Send("421 Can't create socket");
				break;
			}
#ifndef NOLAYERS
			if (m_pGssLayer && m_pGssLayer->AuthSuccessful())
				m_transferstatus.socket->UseGSS(m_pGssLayer);
#endif

			if (!m_transferstatus.socket->Listen())
			{
				delete m_transferstatus.socket;
				m_transferstatus.socket = NULL;
				Send("421 Can't create socket");
				break;
			}
			
			//Now retrieve the port
			SOCKADDR_IN sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			
			int nSockAddrLen = sizeof(sockAddr);
			BOOL bResult = m_transferstatus.socket->GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				port = ntohs(sockAddr.sin_port);
			//Reformat the ip
			ip.Replace(".",",");
			//Put the answer together
			CStdString str;
			str.Format("227 Entering Passive Mode (%s,%d,%d)",ip,port/256,port%256);
			Send(str);
			m_transferstatus.pasv=1;
			break;
		}
	case COMMAND_TYPE:
		{
			args.MakeUpper();
			if (args[0] != 'I' && args[0] != 'A')
			{
				Send("501 Parameters invalid. Must be I for binary and A for ASCII type.");
				break;
			}
			m_transferstatus.type=(args[0]=='I')?0:1;
			Send(CStdString("200 Type set to ") + (m_transferstatus.type ? "A" : "I"));
		}
		break;
	case COMMAND_LIST:
		if(m_transferstatus.pasv==-1)
		{
			Send("503 Bad sequence of commands.");
			break;
		}
		if(!m_transferstatus.pasv && (m_transferstatus.ip=="" || m_transferstatus.port==-1))
			Send("503 Bad sequence of commands.");
		//Much more checks
		else
		{
			//Check args, currently only supported argument is the directory which will be listed.
			CStdString dirToList=m_CurrentDir;
			args.TrimLeft(" ");
			args.TrimRight(" ");
			if (args!="")	
			{
				BOOL bBreak=FALSE;
				while (args[0]=='-') //No parameters supported
				{
					if (args.GetLength()<2)
					{ //Dash without param
						Send("501 Syntax error");
						bBreak = TRUE;
						break;
					}
					
					int pos=args.Find(" ");
					CStdString params;
					if (pos!=-1)
					{
						params=args.Left(1);
						params=params.Left(pos-1);
						args=args.Mid(pos+1);
						args.TrimLeft(" ");
					}
					else
						args="";
					while (params!="")
					{
						//Some parameters are not support
						if (params[0]=='R')
						{
							Send("504 Command not implemented for that parameter");
							bBreak = TRUE;
							break;
						}
						//Ignore other parameters
						params=params.Mid(1);
					}

					if (args=="")
						break;
				}
				if (bBreak)
					break;
				if (args!="")
				{
					//Unquote args
					if (!UnquoteArgs(args))
					{
						Send( _T("501 Syntax error") );
						break;
					}

					args.Replace("\\","/");
					args.Replace("//", "/");
					//Convert relative vpath into absolute vpath
					if (args[0]!='/')
						args=m_CurrentDir+"/"+args;
					dirToList=args;
				}
			}
			
			t_dirlisting *pResult;
#if defined(_XBOX)
			// in case of xbox => make a lookup for the user and check the users homedir and relative-mode
			// if "not relative" and "/" then display the whole permission-tree
			int error;

			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {

				t_dirlisting *pCurrent = NULL;

				pResult = new t_dirlisting;
				pResult->len = 0;
				pResult->pNext=NULL;

				CStdString result;
				result="";

				t_directory dir;
				for (std::vector<t_directory>::const_iterator iter=user.permissions.begin(); iter!=user.permissions.end(); iter++) {
					if((iter->dir != "/") && (iter->dir != "\\")) {

						CStdString dirToList = iter->dir;

						if (isalpha(iter->dir[0]) && iter->dir[1] == ':')
						{
							char drive = tolower(iter->dir[0]);
							if (drive >= 'f' && drive < 'q')
							{
								// extended partitions and memory units - check if the drive is available
								if (!CIoSupport::DriveExists(drive) && !g_memoryUnitManager.IsDriveValid(drive))
									continue;
							}
							// don't show x, y, z in the listing as users shouldn't really be
							// stuffing around with these drives (power users can always go
							// to these folders by specifying the path directly)
							if (drive >= 'x' && !g_advancedSettings.m_FTPShowCache)
								continue;

							if (1 /*g_stSettings.m_bFTPSingleCharDrives*/)
							{
								// modified to be consistent with other xbox ftp behavior: drive 
								// name is a single character without the ':' at the end
								dirToList = drive;
								dirToList.MakeUpper();
							}
						}

						dirToList.TrimRight("\\");
						dirToList.TrimRight("/");

						if (NULL==pCurrent)
						{
							pCurrent = pResult;
						}
						else
						{
							pCurrent->pNext = new t_dirlisting;
							pCurrent=pCurrent->pNext;
							pCurrent->pNext=NULL;
						}
						result="dr-xr-xr-x    1 ftp      ftp            1 Feb 23 00:00 ";
						result+=dirToList;
						result+="\r\n";
						
						strcpy(pCurrent->buffer, result);
						pCurrent->len = result.length();
					}
				}
				error=0;
			//do something with result
/*				pResult = new t_dirlisting;
				pResult->pNext=NULL;
				strcpy(pResult->buffer, result);
				pResult->len = result.length();*/
			} else
			{
				//error=m_pOwner->m_pPermissions->GetDirectoryListing( m_status.user, dirToList, result );
				error = m_pOwner->m_pPermissions->GetDirectoryListing(m_status.user, dirToList, pResult);
			}
#else
			int error = m_pOwner->m_pPermissions->GetDirectoryListing(m_status.user, dirToList, pResult);
#endif
			if (error & 1)
			{
				Send("550 Permission denied");
				ResetTransferstatus();
			}
			else if (error)
			{
				Send("550 Directory not found");
				ResetTransferstatus();
			}
			else
			{
				if (!m_transferstatus.pasv)
				{
					if (m_transferstatus.socket)
						delete m_transferstatus.socket;
					CTransferSocket *transfersocket=new CTransferSocket(this);
					m_transferstatus.socket=transfersocket;
					transfersocket->Init(pResult, TRANSFERMODE_LIST );

					if (!CreateTransferSocket(transfersocket))
						break;

					Send("150 Opening data channel for directory list.");
				}
				else
				{
					m_transferstatus.socket->Init(pResult, TRANSFERMODE_LIST );
					m_transferstatus.socket->PasvTransfer();
				}
			}
			
		}
		break;
	case COMMAND_REST:
		{
			BOOL error=FALSE;
			for (int i=0;i<args.GetLength();i++)
				if (!isdigit(args[i]))
					error=TRUE;
			if (error)
			{
				Send("501 Bad parameter. Numeric value required");
				break;
			}
			m_transferstatus.rest=_atoi64(args);
			CStdString str;
			str.Format("350 Rest supported. Restarting at %I64d",m_transferstatus.rest);
			Send(str);
		}
		break;
	case COMMAND_CDUP:
	case COMMAND_XCUP:
		{
			CStdString dir="..";
			int res = m_pOwner->m_pPermissions->ChangeCurrentDir(m_status.user,m_CurrentDir,dir);
#if defined(_XBOX)
			// in case of xbox => cwd to "/"  && user's homedir == "/" just set the currentdir to "/"
			// => ignore an eventual error that res returned !!! FIXME !!!
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				CStdString str;
				str.Format("200 CDUP successful. \"XBOX-ROOT(%s)\" is current directory.",m_CurrentDir);
				Send(str);
			} 
			else
			{
				if (!res)
				{
					SendCurDir("200 CDUP successful.",m_CurrentDir);
				}
				else if (res & 1)
				{
					CStdString str;
					str.Format("550 CDUP failed. \"%s\": Permission denied.",dir);
					Send(str);
				}
				else if (res)
				{
					CStdString str;
					str.Format("550 CDUP failed. \"%s\": directory not found.",dir);
					Send(str);
				}
			}
#else
			if (!res)
			{
				CStdString str;
				str.Format("200 CDUP successful. \"%s\" is current directory.",m_CurrentDir);
				Send(str);
			}
			else if (res & 1)
			{
				CStdString str;
				str.Format("550 CDUP failed. \"%s\": Permission denied.",dir);
				Send(str);
			}
			else if (res)
			{
				CStdString str;
				str.Format("550 CDUP failed. \"%s\": directory not found.",dir);
				Send(str);
			}
#endif
		}
		break;
	case COMMAND_RETR:
		{
			if(m_transferstatus.pasv==-1)
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			if(!m_transferstatus.pasv && (m_transferstatus.ip=="" || m_transferstatus.port==-1))
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			//Much more checks
			
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}

		
			CStdString result;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user,args,m_CurrentDir,FOP_READ,result);
			if (error & 1)
			{
				Send("550 Permission denied");
				ResetTransferstatus();
			}
			else if (error)
			{
				Send("550 File not found");
				ResetTransferstatus();
			}
			else
			{
				if (!m_transferstatus.pasv)
				{
					if (m_transferstatus.socket)
						delete m_transferstatus.socket;
					CTransferSocket *transfersocket=new CTransferSocket(this);
					m_transferstatus.socket=transfersocket;
					transfersocket->Init(result, TRANSFERMODE_SEND, m_transferstatus.rest);

					if (!CreateTransferSocket(transfersocket))
						break;
					
					Send("150 Opening data channel for file transfer.");
				}
				else
				{
					m_transferstatus.socket->Init(result,TRANSFERMODE_SEND,m_transferstatus.rest);
					m_transferstatus.socket->PasvTransfer();
				}
				GetSystemTime(&m_LastTransferTime);
			}
			break;	
		}
	case COMMAND_STOR:
		{
			if(m_transferstatus.pasv==-1)
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			if(!m_transferstatus.pasv && (m_transferstatus.ip=="" || m_transferstatus.port==-1))
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			//Much more checks
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Storing in XBOX Root not allowed.") );
				break;
			}
#endif

			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}

			CStdString result;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user,args,m_CurrentDir,m_transferstatus.rest?FOP_APPEND:FOP_WRITE,result);
			if (error & 1)
			{
				Send("550 Permission denied");
				ResetTransferstatus();
			}
			else if (error)
			{
				Send("550 Filename invalid");
				ResetTransferstatus();
			}
			else
			{
				if (!m_transferstatus.pasv)
				{
					CTransferSocket *transfersocket=new CTransferSocket(this);
					transfersocket->Init(result,TRANSFERMODE_RECEIVE,m_transferstatus.rest);
					m_transferstatus.socket=transfersocket;
					
					if (!CreateTransferSocket(transfersocket))
						break;
					Send("150 Opening data channel for file transfer.");
				}
				else
				{
					m_transferstatus.socket->Init(result,TRANSFERMODE_RECEIVE,m_transferstatus.rest);
					m_transferstatus.socket->PasvTransfer();
				}
				GetSystemTime(&m_LastTransferTime);
			}		
		}
		break;
	case COMMAND_SIZE:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}

			CStdString result;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user, args, m_CurrentDir, FOP_READ, result);
			if (error & 1)
				Send("550 Permission denied");
			else if (error)
				Send("550 File not found");
			else 
			{
				CStdString str;
				_int64 length;
				if (GetLength64(result, length))
					str.Format("213 %I64d", length);
				else
					str="550 File not found";
				Send(str);
			}
		}
		break;
	case COMMAND_DELE:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Deleting in XBOX Root not allowed.") );
				break;
			}
#endif

			CStdString result;
			int error=m_pOwner->m_pPermissions->GetFileName(m_status.user,args,m_CurrentDir,FOP_DELETE,result);
			if (error & 1)
				Send("550 Permission denied");
			else if (error)
				Send("550 File not found");
			else
			{
				if (!DeleteFile(result))
					Send(_T("450 Internal error deleting the file."));
				else
					Send(_T("250 File deleted successfully"));
			}
		}
		break;
	case COMMAND_RMD:
	case COMMAND_XRMD:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Directory Deleting in XBOX Root not allowed.") );
				break;
			}
#endif
			CStdString result, logical;
			int error = m_pOwner->m_pPermissions->GetDirName(m_status.user,args,m_CurrentDir,DOP_DELETE,result,logical);
			if (error & 1)
				Send("550 Permission denied");
			else if (error)
				Send("550 Directory not found");
			else
			{
				if (!RemoveDirectory(result))
				{
					if (GetLastError()==ERROR_DIR_NOT_EMPTY)
						Send("550 Directory not empty.");
					else
						Send("450 Internal error deleting the directory.");
				}
				else
					Send("250 Directory deleted successfully");
			}
		}
		break;	
	case COMMAND_MKD:
	case COMMAND_XMKD:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Creating directories in XBOX Root not allowed.") );
				break;
			}
#endif
			CStdString result, logical;
			int error=m_pOwner->m_pPermissions->GetDirName(m_status.user, args,m_CurrentDir, DOP_CREATE, result, logical);
			if (error & PERMISSION_DOESALREADYEXIST && (error & PERMISSION_FILENOTDIR)!=PERMISSION_FILENOTDIR)
				Send("550 Directory already exists");
			else if (error & PERMISSION_DENIED)
				Send("550 Can't create directory. Permission denied");
			else if (error)
				Send("550 Directoryname not valid");
			else
			{
				result+="\\";
				CStdString str;
				BOOL res = FALSE;
				BOOL bReplySent = FALSE;
				while (result!="")
				{
					CStdString piece = result.Left(result.Find("\\")+1);
					if (piece.Right(2) == ".\\")
					{
						Send("550 Directoryname not valid");
						bReplySent = TRUE;
						break;
					}

          str += piece;
					result = result.Mid(result.Find("\\")+1);
					res = CreateDirectory(str,0);
				}
				if (!bReplySent)
					if (!res)
						Send("450 Internal error creating the directory.");
					else
            SendDir("257",logical, " Was created");
			}
		}
		break;
	case COMMAND_RNFR:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Renaming in XBOX Root not allowed.") );
				break;
			}
#endif
			RenName = "";

			CStdString result, logical;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user, args, m_CurrentDir, FOP_DELETE, result);
			if (!error)
			{
				RenName = result;
				bRenFile = TRUE;
				Send("350 File exists, ready for destination name.");
				break;
			}
			else if (error & PERMISSION_DENIED)
				Send("550 Permission denied");
			else
			{
				int error2 = m_pOwner->m_pPermissions->GetDirName(m_status.user, args,m_CurrentDir, DOP_DELETE, result, logical);
				if (!error2)
				{
					RenName=result;
					bRenFile = FALSE;
					Send("350 Directory exists, ready for destination name.");
				}
				else if (error2 & 1)
					Send("550 Permission denied");
				else
					Send("550 file/directory not found");
				break;
			}
		}
		break;
	case COMMAND_RNTO:
		{
			if (RenName=="")
			{
				Send("503 Bad sequence of commands!");
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Renaming in XBOX Root not allowed.") );
				break;
			}
#endif
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}

			if (bRenFile)
			{
				CStdString result;
				int error = m_pOwner->m_pPermissions->GetFileName(m_status.user, args, m_CurrentDir, FOP_CREATENEW, result);
#if defined(_XBOX)
				if (g_guiSettings.GetBool("servers.ftpautofatx"))
					CUtil::GetFatXQualifiedPath(result);
#endif
				if (error)
					RenName = "";
				if (error & 1)
					Send("550 Permission denied");
				else if (error & PERMISSION_DOESALREADYEXIST && (error & PERMISSION_DIRNOTFILE)!=PERMISSION_DIRNOTFILE)
					Send("550 file exists");
				else if (error)
					Send("550 Filename invalid");
				else
				{
					if (!MoveFile(RenName, result))
						Send("450 Internal error renaming the file");
					else
						Send("250 file renamed successfully");
				}
			}
			else
			{
				CStdString result, logical;
				int error = m_pOwner->m_pPermissions->GetDirName(m_status.user, args, m_CurrentDir, DOP_CREATE, result, logical);
#if defined(_XBOX)
				if (g_guiSettings.GetBool("servers.ftpautofatx"))
					CUtil::GetFatXQualifiedPath(result);
#endif       
				if (error)
					RenName = "";
				if (error & 1)
					Send("550 Permission denied");
				else if (error & PERMISSION_DOESALREADYEXIST && (error & PERMISSION_FILENOTDIR)!=PERMISSION_FILENOTDIR)
					Send("550 file exists");
				else if (error)
					Send("550 Filename invalid");
				else
				{
					if (!MoveFile(RenName, result))
						Send("450 Internal error renaming the file");
					else
						Send("250 file renamed successfully");
				}
			}		
		}
		break;
	case COMMAND_ABOR:
		{
			if (m_transferstatus.socket)
			{
				if (m_transferstatus.socket->Started())
					Send("426 Connection closed; transfer aborted.");
			}
#if defined(_XBOX)
      CStdString prompt;
      if (XBFILEZILLA(GetFreeSpacePrompt(226, prompt)))
        Send(prompt.c_str());
#endif
			Send("226 ABOR command successful");
			ResetTransferstatus();
		break;
		}
	case COMMAND_SYST:
		Send("215 UNIX emulated by FileZilla");
		break;
	case COMMAND_NOOP:
	case COMMAND_NOP:
		Send("200 OK");
		break;
	case COMMAND_APPE:
		{
			if(m_transferstatus.pasv==-1)
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			if(!m_transferstatus.pasv && (m_transferstatus.ip=="" || m_transferstatus.port==-1))
			{
				Send("503 Bad sequence of commands.");
				break;
			}
			//Much more checks

			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}
#if defined(_XBOX)
			// in case of xbox => deny all operations apart from cwd in xbox-root
			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				Send( _T("550 Permission denied - Append in XBOX Root not allowed.") );
				break;
			}
#endif
			CStdString result;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user,args,m_CurrentDir,FOP_APPEND,result);
			if (error & 1)
			{
				Send("550 Permission denied");
				ResetTransferstatus();
			}
			else if (error)
			{
				Send("550 Filename invalid");
				ResetTransferstatus();
			}
			else
			{
				_int64 size = 0;
				if (!GetLength64(result, size))
					size = 0;

				m_transferstatus.rest = size;

				if (!m_transferstatus.pasv)
				{
					CTransferSocket *transfersocket = new CTransferSocket(this);
					transfersocket->Init(result,TRANSFERMODE_RECEIVE, m_transferstatus.rest);
					m_transferstatus.socket=transfersocket;
					
					if (!CreateTransferSocket(transfersocket))
						break;

					CStdString str;
					str.Format("150 Opening data channel for file transfer, restarting at offset %I64d",size);
					Send(str);
				}
				else
				{
					m_transferstatus.socket->Init(result,TRANSFERMODE_RECEIVE,m_transferstatus.rest);
					m_transferstatus.socket->PasvTransfer();
				}
				GetSystemTime(&m_LastTransferTime);
			}		
		}
		break;
	case COMMAND_NLST:
		if(m_transferstatus.pasv==-1)
		{
			Send("503 Bad sequence of commands.");
			break;
		}
		if(!m_transferstatus.pasv && (m_transferstatus.ip=="" || m_transferstatus.port==-1))
			Send("503 Bad sequence of commands.");
		//Much more checks
		else
		{
			//Check args, currently only supported argument is the directory which will be listed.
			args.TrimLeft(" ");
			args.TrimRight(" ");
			if (args!="")	
			{
				BOOL bBreak=FALSE;
				while (args[0]=='-') //No parameters supported
				{
					if (args.GetLength()<2)
					{ //Dash without param
						Send("501 Syntax error");
						bBreak = TRUE;
						break;
					}
					
					int pos=args.Find(" ");
					CStdString params;
					if (pos!=-1)
					{
						params=args.Left(1);
						params=params.Left(pos-1);
						args=args.Mid(pos+1);
						args.TrimLeft(" ");
					}
					else
						args="";
					while (params!="")
					{
						//Some parameters are not support
						if (params[0]=='R')
						{
							Send("504 Command not implemented for that parameter");
							bBreak = TRUE;
							break;
						}
						//Ignore other parameters
						params=params.Mid(1);
					}

					if (args=="")
						break;
				}
				if (bBreak)
					break;
				if (args!="")
				{
					//Unquote args
					if (!UnquoteArgs(args))
					{
						Send( _T("501 Syntax error") );
						break;
					}

					args.Replace("\\","/");
					args.Replace("//", "/");
					args.TrimRight("/");
				}
			}
			
			t_dirlisting *pResult;
#if defined(_XBOX)
			// in case of xbox => make a lookup for the user and check the users homedir and relative-mode
			// if "not relative" and "/" then display the whole permission-tree
			int error;

			CUser user;
			m_pOwner->m_pPermissions->GetUser(m_status.user,user);
			if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {

				t_dirlisting *pCurrent = NULL;

				pResult = new t_dirlisting;
				pResult->len = 0;
				pResult->pNext=NULL;

				CStdString result;
				result="";

				t_directory dir;
				for (std::vector<t_directory>::const_iterator iter=user.permissions.begin(); iter!=user.permissions.end(); iter++) {
					if ((iter->dir != "/") && (iter->dir != "\\")) {
						if (NULL==pCurrent)
						{
							pCurrent = pResult;
						}
						else
						{
							pCurrent->pNext = new t_dirlisting;
							pCurrent=pCurrent->pNext;
							pCurrent->pNext=NULL;
						}

            if (1 /*g_stSettings.m_bFTPSingleCharDrives*/
              && (iter->dir.GetLength() == 3) 
              && isalpha(iter->dir[0]) && (iter->dir[1] == ':') && (iter->dir[2] == '\\'))
            {
              // modified to be consistent with other xbox ftp behavior: drive 
              // name is a single character without the ':' at the end
              result=iter->dir[0];
            }
            else
            {
						result=iter->dir;
            }
						result+="\r\n";

						strcpy(pCurrent->buffer, result);
						pCurrent->len = result.length();
					}
				}
				error=0;

				//do something with result

				/*pResult = new t_dirlisting;
				pResult->pNext=NULL;
				strcpy(pResult->buffer, result);
				pResult->len = result.length();*/
			} else
			{
				//error=m_pOwner->m_pPermissions->GetShortDirectoryListing( m_status.user, dirToList, result );
				error = m_pOwner->m_pPermissions->GetShortDirectoryListing(m_status.user, m_CurrentDir, args, pResult);
			}
#else
			int error = m_pOwner->m_pPermissions->GetShortDirectoryListing(m_status.user, m_CurrentDir, args, pResult);
#endif
			if (error & 1)
			{
				Send("550 Permission denied");
				ResetTransferstatus();
			}
			else if (error)
			{
				Send("550 Directory not found");
				ResetTransferstatus();
			}
			else
			{
				if (!m_transferstatus.pasv)
				{
					CTransferSocket *transfersocket = new CTransferSocket(this);
					m_transferstatus.socket = transfersocket;
					transfersocket->Init(pResult, TRANSFERMODE_NLST);

					if (!CreateTransferSocket(transfersocket))
						break;
					
					Send("150 Opening data channel for directory list.");
				}
				else
				{
					m_transferstatus.socket->Init(pResult, TRANSFERMODE_NLST );
					m_transferstatus.socket->PasvTransfer();
				}
			}
			
		}
		break;
	case COMMAND_MDTM:
		{
			//Unquote args
			if (!UnquoteArgs(args))
			{
				Send( _T("501 Syntax error") );
				break;
			}

			CStdString result;
			int error = m_pOwner->m_pPermissions->GetFileName(m_status.user, args, m_CurrentDir, FOP_READ, result);
			if (error & 1)
				Send("550 Permission denied");
			else if (error & 2)
				Send("550 File not found");
			else 
			{
				CFileStatus64 status;
				GetStatus64(result,status);
				status.m_mtime;
				CStdString str;
				SYSTEMTIME time;
				FileTimeToSystemTime(&status.m_mtime, &time);
				str.Format("213 %04d%02d%02d%02d%02d%02d",
							time.wYear,
							time.wMonth,
							time.wDay,
							time.wHour,
							time.wMinute,
							time.wSecond);
				Send(str);
			}
		}
		break;
	case COMMAND_EPSV:
		{
			if (m_transferstatus.socket)
				delete m_transferstatus.socket;
			m_transferstatus.socket=new CTransferSocket(this);

			unsigned int port = 0;
			unsigned int retries = 3;
			while (retries > 0) {
				if (m_pOwner->m_pOptions->GetOptionVal(OPTION_USECUSTOMPASVPORT)) {
					static UINT customPort = 0;
					unsigned int minPort = (unsigned int)m_pOwner->m_pOptions->GetOptionVal(OPTION_CUSTOMPASVMINPORT);
					unsigned int maxPort = (unsigned int)m_pOwner->m_pOptions->GetOptionVal(OPTION_CUSTOMPASVMAXPORT);
					if (minPort > maxPort) {
						unsigned int temp = minPort;
						minPort = maxPort;
						maxPort = temp;
					}
					if (customPort < minPort || customPort > maxPort) {
						customPort = minPort;
					}
					port = customPort;
					++customPort;
				}
				if (m_transferstatus.socket->Create(port, SOCK_STREAM, FD_ACCEPT))
				{
					break;
				}
				--retries;
			}
			if (retries <= 0) {
				delete m_transferstatus.socket;
				m_transferstatus.socket=0;
				Send("421 Can't create socket");
				break;
			}
#ifndef NOLAYERS
			if (m_pGssLayer && m_pGssLayer->AuthSuccessful())
				m_transferstatus.socket->UseGSS(m_pGssLayer);
#endif
	
			if (!m_transferstatus.socket->Listen())
			{
				delete m_transferstatus.socket;
				m_transferstatus.socket=0;
				Send("421 Can't create socket");
				break;
			}
			
			//Now retrieve the port
			SOCKADDR_IN sockAddr;
			memset(&sockAddr, 0, sizeof(sockAddr));
			
			int nSockAddrLen = sizeof(sockAddr);
			BOOL bResult = m_transferstatus.socket->GetSockName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if (bResult)
				port = ntohs(sockAddr.sin_port);
			//Put the answer together
			CStdString str;
			str.Format("229 Entering Extended Passive Mode (|||%d|)", port);
			Send(str);
			m_transferstatus.pasv=1;
			break;
		}
	case COMMAND_EPRT:
		{
			if (m_transferstatus.socket)
			{
				delete m_transferstatus.socket;
				m_transferstatus.socket=0;
			}
		
			if (args[0] != '|')
			{
				Send("501 Syntax error");
				break;
			}
			args = args.Mid(1);

			int pos = args.Find('|');
			if (pos < 1 || (pos>=(args.GetLength()-1)))
			{
				Send("501 Syntax error");
				break;
			}
			int protocol = _ttoi(args.Left(pos));
			if (protocol != 1)
			{
				Send("522 Extended Port Failure - unknown network protocol");
				break;
			}
			args = args.Mid(pos + 1);

			pos = args.Find('|');
			if (pos < 1 || (pos>=(args.GetLength()-1)))
			{
				Send("501 Syntax error");
				break;
			}
			CStdString ip = args.Left(pos);
			if (inet_addr(ip) == INADDR_NONE)
			{
				Send("501 Syntax error");
				break;
			}
			args = args.Mid(pos + 1);

			pos = args.Find('|');
			if (pos < 1)
			{
				Send("501 Syntax error");
				break;
			}
			int port = _ttoi(args.Left(pos));
			if (port<1 || port>65535)
			{
				Send("501 Syntax error");
				break;
			}

			m_transferstatus.port = port;
			m_transferstatus.ip = ip;

			m_transferstatus.pasv=0;
			Send("200 Port command successful");
			break;
		}
	case COMMAND_AUTH:
		{
#ifndef NOLAYERS
			if (m_pGssLayer)
			{
				Send("534 Authentication type already set to GSSAPI");
				break;
			}
			else
#endif
			{
				args.MakeLower();
				if (args != _T("gssapi"))
				{
					Send("504 Auth type not supported");
					break;
				}

				if (!m_pOwner->m_pOptions->GetOptionVal(OPTION_USEGSS))
				{
					Send("502 GSSAPI authentication not implemented");
					break;
				}
#ifndef NOLAYERS
				m_pGssLayer = new CAsyncGssSocketLayer;
				BOOL res = AddLayer(m_pGssLayer);
				if (res)
				{
					res = m_pGssLayer->InitGSS(FALSE, (BOOL)m_pOwner->m_pOptions->GetOptionVal(OPTION_GSSPROMPTPASSWORD));
					if (!res)				
						SendStatus("Unable to init GSS", 1);
				}
				if (!res)
				{
					RemoveAllLayers();
					delete m_pGssLayer;
					m_pGssLayer = NULL;
					Send("431 Could not initialize GSSAPI libraries");
					break;
				}

				Send("334 Using authentication type GSSAPI; ADAT must follow");
			//	XXX
#endif
			}
			break;
		}
#ifndef NOLAYERS
	case COMMAND_ADAT:
	case COMMAND_PBSZ:
	case COMMAND_PROT:
		{
			if (!m_pGssLayer)
			{
				Send("502 Command not implemented");
				break;
			}

			char sendme[4096];
			
			char command1[4096];
			char args1[4096];
			
			strcpy(command1, command);
			strupr(command1);
			strcpy(args1, args);
			
			m_pGssLayer->ProcessCommand(command1, args1, sendme);
			Send(sendme);
			
			break;
		}
#endif

#if defined(_XBOX)
  case COMMAND_SITE:
    {
      //Get command
	    CStdString sitecommand, siteargs;
	    
	    if (!GetCommandFromString(args, sitecommand, siteargs))
		    return;

      CStdString fullcommand = sitecommand;
      if (siteargs.size() > 0) fullcommand += "(" + siteargs + ")";
      
      CLog::Log(LOGNOTICE, "200 FTP SITE command called [command=%s, args=%s]", sitecommand.c_str(), siteargs.c_str());

	    //Check if command is valid
	    int nCommandID = -1;
/*	    for (int i = 0; i < (sizeof(site_commands) / sizeof(t_command)); i++)
	    {
		    if (sitecommand == site_commands[i].command)
		    {
			    //Does the command needs an argument?
			    if (site_commands[i].bHasargs && (siteargs == ""))
			    {
				    Send("501 Syntax error");
				    return;
			    }
			    //Can it be issued before logon?
			    else 
          if (!m_status.loggedon && !site_commands[i].bValidBeforeLogon)
			    {
				    Send("530 Please log in with USER and PASS first.");
				    return;
			    }
			    nCommandID = site_commands[i].nID;
			    break;			
		    }
	    }*/
	    //Command not recognized
	    if (nCommandID==-1)
	    {
        // check for a built-in function
        CStdString strBuiltIn = fullcommand;
        if (!CUtil::IsBuiltIn(fullcommand))
          strBuiltIn = "XBMC." + fullcommand;
        if (!CUtil::IsBuiltIn(strBuiltIn))
        { // invalid - send error
          Send(_T("500 Invalid built-in function.  Use SITE HELP for a list of valid SITE commands"));
          return;
        }
        if (strBuiltIn.Equals("xbmc.help", false) || strBuiltIn.Equals("help", false))
        {
          CStdString strHelp;
          CUtil::GetBuiltInHelp(strHelp);
          Send(_T("200-FTP SITE HELP"));
          int iReturn = strHelp.Find("\n");
          while (iReturn >= 0)
          {
            CStdString helpline = "  " + strHelp.Left(iReturn);

            // replace tab with spaces (tab position = 30)
            //   because ftp command line (at least on windows) does 
            //   not show tabs well.
            int iTabPos = helpline.Find("\t");
            if (iTabPos >= 0)
            {
              helpline = helpline.Left(iTabPos) + CStdString(30 - iTabPos, ' ') + helpline.Mid(iTabPos + 1);
            }
            
            Send(_T(helpline.c_str()));
            strHelp = strHelp.Mid(iReturn + 1);
            iReturn = strHelp.Find("\n");
          }
          Send(_T("200 End of help"));
        }
        else
        {
          // send using a threadmessage...
          ThreadMessage tMsg = {TMSG_EXECUTE_BUILT_IN};
          tMsg.strParam = fullcommand;
          g_applicationMessenger.SendMessage(tMsg, true);
          Send(_T("200 Executed built in function."));
        }
		    return;
	    }
      switch (nCommandID)
      {
        case -1:
        {
	        // not recognized as a standard command, call ExecBuiltIn
          if (sitecommand.Find(".") != 5)
          {
            CStdString prefix("XBOX.");
            sitecommand = prefix + sitecommand;
          }

          {
	          CStdString str;
	          str.Format("200 FTP SITE - calling ExecBuiltIn [command=%s, args=%s]", sitecommand.c_str(), siteargs.c_str());
	          //Send(str);
            CLog::Log(LOGNOTICE, str);
          }

          int rtn = CUtil::ExecBuiltIn(siteargs);

          {
	          CStdString str;
	          str.Format("200 FTP SITE - called ExecBuiltIn [command=%s, args=%s, rtn=%i]", sitecommand.c_str(), siteargs.c_str(), rtn);
	          //Send(str);
            CLog::Log(LOGNOTICE, str);
          }

          if (rtn == 0)
          {
            Send("226 Command executed successfully.");
          }
          else if (rtn == -1)
          {
		        Send("500 Syntax error, command unrecognized.");
          }
          else
          {
		        Send("550 Command error.");
          }
		      break;
        }
        case SITE_CRC:
          {
			    // in case of xbox => deny all operations apart from cwd in xbox-root
			    /*
          CUser user;
			    m_pOwner->m_pPermissions->GetUser(m_status.user,user);

			    if((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) {
				    Send( _T("550 Permission denied - Storing in XBOX Root not allowed.") );
				    break;
			    }
          */

			      //Unquote args
			      if (!UnquoteArgs(siteargs))
			      {
				      Send( _T("501 Syntax error") );
				      break;
			      }

			      CStdString filename;
			      int error=m_pOwner->m_pPermissions->GetFileName(m_status.user,siteargs,m_CurrentDir,FOP_READ,filename);
			      if (error&1)
			      {
				      Send("550 Permission denied");
				      ResetTransferstatus();
			      }
			      else if (error&14)
			      {
				      Send("550 File not found");
				      ResetTransferstatus();
			      }
			      else
			      {
              Send(_T("200- Please wait, calculating CRC..."));
              CStdString prompt;
              unsigned long crc = 0;  
              XFSTATUS result = XBFILEZILLA(GetFileCRC(filename, crc));
              if (result == XFS_OK)
                prompt.Format(_T("200 File \"%s\", CRC 0x%08X"), filename.c_str(), crc);
              else
                prompt.Format(_T("200 File \"%s\", CRC Fatal error: cannot read file"), filename.c_str());

              Send(prompt.c_str());
            }
          }
          break;
        case SITE_EXECUTE:
          {
			      // in case of xbox => deny all operations apart from cwd in xbox-root
          /*
			      CUser user;
			      m_pOwner->m_pPermissions->GetUser(m_status.user, user);

			      if ((m_CurrentDir=="/") && (user.nRelative == FALSE) && (m_pOwner->m_pPermissions->GetHomeDir(m_status.user) == "/")) 
            {
				      Send( _T("550 Permission denied - Executing in XBOX Root not allowed.") );
				      break;
			      }
            */

			      //Unquote args
			      if (!UnquoteArgs(siteargs))
			      {
				      Send( _T("501 Syntax error") );
				      break;
			      }

			      CStdString result;
			      int error = m_pOwner->m_pPermissions->GetFileName(m_status.user, siteargs, m_CurrentDir, FOP_READ, result);
			      if (error&1)
			      {
				      Send("550 Permission denied");
				      ResetTransferstatus();
			      }
			      else if (error&14)
			      {
				      Send("550 File not found");
				      ResetTransferstatus();
			      }
            else
			      {
				      // launch the file
              XFSTATUS status = XBFILEZILLA(LaunchXBE(result));
              if (status == XFS_OK) 
              {
                CStdString prompt;
                prompt.Format(_T("200 Launching %s"), result);
                Send(prompt.c_str());
              }
              else
              if (status == XFS_NOT_EXECUTABLE)
                Send("550 File is not executable");
              else
              if (status == XFS_NOT_FOUND)
                Send("550 File not found");
              else
              if (status == XFS_NOT_IMPLEMENTED)
                Send("550 Not implemented");
              else
                Send("550 Unknown error");
              ResetTransferstatus();
            }
          }
          break;
    
/*        case SITE_REBOOT:
          {
            OutputDebugString(_T("Rebooting...\n"));
            XFSTATUS result = XBFILEZILLA(Reboot());
            if (result == XFS_OK) 
              Send("550 Should not get here");
            else
            if (result == XFS_NOT_IMPLEMENTED)
              Send("550 Not implemented");
            else
              Send("550 Unknown error");
            ResetTransferstatus();
          }

          break;
        case SITE_SHUTDOWN:
          {
            OutputDebugString(_T("Shutdown...\n"));
            XFSTATUS result = XBFILEZILLA(Shutdown());
            if (result == XFS_OK) 
              Send("550 Should not get here");
            else
            if (result == XFS_NOT_IMPLEMENTED)
              Send("550 Not implemented");
            else
              Send("550 Unknown error");
            ResetTransferstatus();
          }
          break;
*/
        default:
    		  Send("502 Command not implemented.");
      };
    }
    break;
#endif
	default:
		Send("502 Command not implemented.");
	}
	return;
}

void CControlSocket::ProcessTransferMsg()
{
	if (!m_transferstatus.socket)
		return;
	int status=m_transferstatus.socket->GetStatus();
	
	GetSystemTime(&m_LastCmdTime);
	if (m_transferstatus.socket)
		if (m_transferstatus.socket->GetMode()==TRANSFERMODE_SEND || m_transferstatus.socket->GetMode()==TRANSFERMODE_RECEIVE)
			GetSystemTime(&m_LastTransferTime);

	if (status == 2 && m_transferstatus.pasv)
		m_pOwner->ExternalIPFailed();

#if defined(_XBOX)

  if (m_transferstatus.socket->m_nMode == TRANSFERMODE_SEND ||
      m_transferstatus.socket->m_nMode == TRANSFERMODE_RECEIVE)
  {
    SXFTransferInfo* info = new SXFTransferInfo;
    info->mConnectionId = m_userid;
    info->mFilename = m_transferstatus.socket->m_Filename.c_str();
    info->mIsResumed = (m_transferstatus.socket->m_nRest > 0);
    if (m_transferstatus.socket->m_nMode == TRANSFERMODE_SEND)
      info->mStatus = (status == 0 ? SXFTransferInfo::sendEnd : SXFTransferInfo::sendError);
    else
    if (m_transferstatus.socket->m_nMode == TRANSFERMODE_RECEIVE)
      info->mStatus = (status == 0 ? SXFTransferInfo::recvEnd : SXFTransferInfo::recvError);

    if (!PostMessage(hMainWnd, WM_FILEZILLA_SERVERMSG, FSM_FILETRANSFER, (LPARAM)info))
      delete info;
  }

  bool sendStats = (m_transferstatus.socket->m_nMode == TRANSFERMODE_RECEIVE);
  CStdString filename = m_transferstatus.socket->m_Filename;
#endif  

	delete m_transferstatus.socket;
	m_transferstatus.socket=0;
	m_transferstatus.ip="";
	m_transferstatus.port=-1;
	m_transferstatus.pasv=-1;
	m_transferstatus.rest=0;
	m_transferstatus.type=-1;

	if (!status)
{
		
#if defined(_XBOX)
    CStdString prompt;

    if (sendStats)
    {
      unsigned long crc = 0;
      bool crcCalculated = false;
      
      if (XBFILEZILLA(GetSfvEnabled()))
      {
        if (!filename.Right(4).CompareNoCase(_T(".sfv")))
        {
          // this is a sfv file, so don't check
          //mSfvFile.SetSfvFile(filename);
          crcCalculated = false;
        }
        else
        {
          Send(_T("226- Please wait, checking SFV..."));
          CRCSTATUS crcresult = mSfvFile.CheckFile(filename, crc);
          switch (crcresult)
          {
            case CRC_OK:
              crcCalculated = true;
              Send(_T("226- File is OK."));
              break;  
            case CRC_NO_SFV:
              crcCalculated = false;
              Send(_T("226- No SFV file available."));
              break;
            case CRC_BAD:
              {
                crcCalculated = true;
                CStdString renamedfile = filename + _T(".bad");
                if (!rename(filename.c_str(), renamedfile.c_str()))
                  Send(_T("226- File has bad CRC, file has been renamed."));
                else
                  Send(_T("226- File has bad CRC, error while renaming file."));
              }
              break;
            case CRC_MISSING:
              {
                crcCalculated = false;
                CStdString renamedfile = filename + _T(".missing");
                if (!rename(filename.c_str(), renamedfile.c_str()))
                  Send(_T("226- File missing in SFV file, file has been renamed."));
                else
                  Send(_T("226- File missing in SFV file, error while renaming file."));
              }
              break;
            case CRC_ERROR:
            default:
              crcCalculated = false;
              Send(_T("226- Error while checking SFV."));
              break;
          };
        }
      }
      

      if (XBFILEZILLA(GetCrcEnabled()))
      {
        Send(_T("226- Please wait, calculating CRC..."));
        
        XFSTATUS result;
        if (crcCalculated)
          result = XFS_OK;
        else
          result = XBFILEZILLA(GetFileCRC(filename, crc));

        if (result == XFS_OK)
          prompt.Format(_T("226- File \"%s\", CRC 0x%08X"), filename.c_str(), crc);
        else
          prompt.Format(_T("226- File \"%s\", CRC Fatal error: cannot read file"), filename.c_str());

        Send(prompt.c_str());
      }
    }

    if (XBFILEZILLA(GetFreeSpacePrompt(226, prompt)))
      Send(prompt.c_str());
#endif
		Send("226 Transfer OK");
	}
	else if (status==1)
		Send("426 Connection closed; transfer aborted.");
	else if (status==2)
		Send("425 Can't open data connection.");
	else if (status==3)
		Send("450 can't access file.");
	else if (status==4)
	{
		Send("426 Connection timed out, aborting transfer");
		ForceClose(1);
		return;
	}
	else if (status==5)
		Send("425 Can't open data connection");
#if defined(_XBOX)
	else if (status==6)
		Send("451 Out of memory.");
#endif
	if (status>=0 && m_bWaitGoOffline)
		ForceClose(0);
	else if (m_bQuitCommand)
		ForceClose(5);
	
}

CTransferSocket* CControlSocket::GetTransferSocket()
{
	return m_transferstatus.socket;
}

void CControlSocket::ForceClose(int nReason)
{
	if (m_transferstatus.socket)
	{
		m_transferstatus.socket->Close();
		delete m_transferstatus.socket;
		m_transferstatus.socket=0;
	}
	if (!nReason)
		Send("421 Server is going offline");
	else if (nReason==1)
		Send("421 Connection timed out.");
	else if (nReason==2)
		Send("421 No-transfer-time exceeded. Closing control connection.");
	else if (nReason==3)
		Send("421 Login time exceeded. Closing control connection.");
	else if (nReason==4)
		Send("421 Kicked by Administrator");
	else if (nReason==5)
		Send("221 Goodbye");
	SendStatus("disconnected.",0);	
	Close();
	m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG,FTM_DELSOCKET,m_userid);

}

void CControlSocket::IncUserCount(const CStdString &user)
{
	int curcount=GetUserCount(user)+1;
	m_Sync.Lock();
	m_UserCount[user]=curcount;
	m_Sync.Unlock();
}

void CControlSocket::DecUserCount(const CStdString &user)
{
	int curcount=GetUserCount(user)-1;
	if (curcount<0)
		return;
	m_Sync.Lock();
	m_UserCount[user]=curcount;
	m_Sync.Unlock();
}

int CControlSocket::GetUserCount(const CStdString &user)
{
	m_Sync.Lock();
	int count=0;
	std::map<CStdString, int>::iterator iter = m_UserCount.find(user);
	if (iter!=m_UserCount.end())
		count = iter->second;
	m_Sync.Unlock();
	return count;
}

void CControlSocket::CheckForTimeout()
{
	if (m_transferstatus.socket)
	{
		if(m_transferstatus.socket->CheckForTimeout())
			return;
	}
	_int64 timeout=m_pOwner->m_pOptions->GetOptionVal(OPTION_TIMEOUT);
	if (!timeout)
		return;
	SYSTEMTIME sCurrentTime;
	GetSystemTime(&sCurrentTime);
	FILETIME fCurrentTime;
	SystemTimeToFileTime(&sCurrentTime, &fCurrentTime);
	FILETIME fLastTime;
	SystemTimeToFileTime(&m_LastCmdTime, &fLastTime);
	_int64 elapsed = ((_int64)(fCurrentTime.dwHighDateTime - fLastTime.dwHighDateTime) << 32) + fCurrentTime.dwLowDateTime - fLastTime.dwLowDateTime;
	if (elapsed > (timeout*10000000))
	{
		ForceClose(1);
	}
	if (m_status.loggedon)
	{ //Transfer timeout
		_int64 nNoTransferTimeout=m_pOwner->m_pOptions->GetOptionVal(OPTION_NOTRANSFERTIMEOUT);
		if (!nNoTransferTimeout)
			return;	
		SystemTimeToFileTime(&m_LastTransferTime, &fLastTime);
		elapsed = ((_int64)(fCurrentTime.dwHighDateTime - fLastTime.dwHighDateTime) << 32) + fCurrentTime.dwLowDateTime - fLastTime.dwLowDateTime;
		if (elapsed>(nNoTransferTimeout*10000000))
		{
			ForceClose(2);
		}
	}
	else
	{ //Login timeout
		_int64 nLoginTimeout=m_pOwner->m_pOptions->GetOptionVal(OPTION_LOGINTIMEOUT);
		if (!nLoginTimeout)
			return;	
		SystemTimeToFileTime(&m_LoginTime, &fLastTime);
		elapsed = ((_int64)(fCurrentTime.dwHighDateTime - fLastTime.dwHighDateTime) << 32) + fCurrentTime.dwLowDateTime - fLastTime.dwLowDateTime;
		if (elapsed>(nLoginTimeout*10000000))
		{
			ForceClose(3);
		}
	}
}

void CControlSocket::WaitGoOffline()
{
	if (m_transferstatus.socket)
	{
		if (!m_transferstatus.socket->Started())
			ForceClose(0);
		else
			m_bWaitGoOffline=TRUE;
	}
	else
		ForceClose(0);

}

void CControlSocket::ResetTransferstatus()
{
	if (m_transferstatus.socket)
		delete m_transferstatus.socket;
	m_transferstatus.socket=0;
	m_transferstatus.ip="";
	m_transferstatus.port=-1;
	m_transferstatus.pasv=-1;
	m_transferstatus.rest=0;
	m_transferstatus.type=-1;
}

BOOL CControlSocket::UnquoteArgs(CStdString &args)
{
	args.TrimLeft( _T(" ") );
	args.TrimRight( _T(" ") );
	int pos1=args.Find('"');
	int pos2=args.ReverseFind('"');
	if (pos1==-1 && pos2==-1)
		return TRUE;
	if (pos1 || pos2!=(args.GetLength()-1) || pos1>=(pos2-1))
		return FALSE;
	args=args.Mid(1, args.GetLength()-2);
	if (args.Find('"')!=-1 || args.Left(1)==" " || args.Right(1)==" ")
		return FALSE;
	return TRUE;
}

void CControlSocket::OnSend(int nErrorCode)
{
	if (m_nSendBufferLen && m_pSendBuffer)
	{
		int nLimit = GetSpeedLimit(0);
		if (!nLimit)
			return;
		int numsend = nLimit;
		if (nLimit == -1 || nLimit > m_nSendBufferLen)
			numsend = m_nSendBufferLen;
			

		int numsent = CAsyncSocketEx::Send(m_pSendBuffer, numsend);

		if (numsent==SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
			return;
		if (!numsent || numsent == SOCKET_ERROR)
		{
			Close();
			SendStatus("could not send reply, disconnected.",0);	
			m_pOwner->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_DELSOCKET, m_userid);

			delete [] m_pSendBuffer;
			m_pSendBuffer = NULL;
			m_nSendBufferLen = 0;
			
			return;
		}

		if (nLimit != -1)
			m_SlQuota.nDownloaded += numsent;

		if (numsent == m_nSendBufferLen)
		{
			delete [] m_pSendBuffer;
			m_pSendBuffer = NULL;
			m_nSendBufferLen = 0;
		}
		else
		{
			char *tmp = m_pSendBuffer;
			m_pSendBuffer = new char[m_nSendBufferLen-numsent];
			memcpy(m_pSendBuffer, tmp+numsent, m_nSendBufferLen-numsent);
			delete [] tmp;
			m_nSendBufferLen -= numsent;
			TriggerEvent(FD_WRITE);
		}
	}
}

BOOL CControlSocket::DoUserLogin(char* sendme)
{
	CUser user;
	if (m_pOwner->m_pPermissions->Lookup(m_status.user, "", user, TRUE))
	{
		if (!user.BypassUserLimit())
		{
			int nMaxUsers = (int)m_pOwner->m_pOptions->GetOptionVal(OPTION_MAXUSERS);
			if (m_pOwner->GetGlobalNumConnections()>nMaxUsers&&nMaxUsers)
			{
				SendStatus("Refusing connection. Reason: Max. connection count reached.",1);
				strcpy(sendme, "421 Too many users are connected, please try again later.");
				ForceClose(-1);
				return FALSE;
			}
		}
		
		else if (user.GetUserLimit() && GetUserCount(m_status.user)>=user.GetUserLimit())
		{
				CStdString str;
				str.Format("Refusing connection. Reason: Max. connection count reached for the user \"%s\".",m_status.user);
				SendStatus(str,1);
				strcpy(sendme, "421 Too many users logged in for this account. Try again later.");
				ForceClose(-1);
				return FALSE;
		}

		CStdString ip;
		unsigned int nPort;
				
		SOCKADDR_IN sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		int nSockAddrLen = sizeof(sockAddr);
		BOOL bResult = GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
		if (bResult)
		{
			nPort = ntohs(sockAddr.sin_port);
			ip = inet_ntoa(sockAddr.sin_addr);
		}
				
		int count=m_pOwner->GetIpCount(ip);
		if (user.nIpLimit && count>=user.GetIpLimit())
		{
			CStdString str;
			if (count==1)
				str.Format("Refusing connection. Reason: No more connections allowed from your IP. (%s already connected once)",ip);
			else
				str.Format("Refusing connection. Reason: No more connections allowed from your IP. (%s already connected %d times)",ip,count);
			SendStatus(str,1);
			strcpy(sendme, "421 Refusing connection. No more connections allowed from your IP.");
			ForceClose(-1);
			return FALSE;
		}
	
		m_CurrentDir = m_pOwner->m_pPermissions->GetHomeDir(m_status.user);
		if (m_CurrentDir=="")
		{
			strcpy(sendme, "550 Could not get home dir!");
			ForceClose(-1);
			return FALSE;
		}
				
		m_status.ip=ip;

		count=GetUserCount(user.user);
		if (user.GetUserLimit() && count>=user.GetUserLimit())
		{
			CStdString str;
			str.Format("Refusing connection. Reason: Maximum connection count (%d) reached for this user", user.GetUserLimit());
			SendStatus(str,1);
			sprintf(sendme, "421 Refusing connection. Maximum connection count reached for the user '%s'", user.user);
			Send(str);
			ForceClose(-1);
			return FALSE;
		}

		m_pOwner->IncIpCount(ip);
		IncUserCount(m_status.user);
		m_status.loggedon=TRUE;
	
		GetSystemTime(&m_LastTransferTime);

		m_pOwner->m_pPermissions->AutoCreateDirs(m_status.user);

		t_connectiondata *conndata=new t_connectiondata;
		t_connop *op=new t_connop;

    memset(conndata, 0, sizeof(t_connectiondata));
    memset(op, 0, sizeof(t_connop));

		op->data=conndata;
		op->op=USERCONTROL_CONNOP_MODIFY;
		conndata->userid=m_userid;
		conndata->user = m_status.user;
		conndata->pThread=m_pOwner;

		memset(&sockAddr, 0, sizeof(sockAddr));
		nSockAddrLen = sizeof(sockAddr);
		bResult = GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
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
	}
			
	else 
	{
		strcpy(sendme, "530 Login or password incorrect!");
		return FALSE;
	}

	return TRUE;
}

void CControlSocket::Continue()
{
	if (m_SlQuota.bContinueDownload)
	{
		TriggerEvent(FD_WRITE);
		if (m_transferstatus.socket && m_transferstatus.socket->Started())
			m_transferstatus.socket->TriggerEvent(FD_WRITE);
		m_SlQuota.bContinueDownload = FALSE;
	}

	if (m_SlQuota.bContinueUpload)
	{
		TriggerEvent(FD_READ);
		if (m_transferstatus.socket && m_transferstatus.socket->Started())
			m_transferstatus.socket->TriggerEvent(FD_READ);
		m_SlQuota.bContinueUpload = FALSE;
	}
}

int CControlSocket::GetSpeedLimit(int nMode)
{
	if (nMode)
	{
		CUser user;
		int nLimit = -1;
		if (m_status.loggedon && m_pOwner->m_pPermissions->GetUser(m_status.user, user))
		{
			nLimit = user.GetCurrentUploadSpeedLimit();
		}
		if (nLimit > 0)
		{
			nLimit *= 100;
			if (m_SlQuota.nUploaded >= nLimit)
			{
				m_SlQuota.bContinueUpload = TRUE;
				return 0;
			}
			else
				nLimit -= m_SlQuota.nUploaded;
		}
		else
			nLimit = -1;
		if (user.BypassServerUploadSpeedLimit())
			m_SlQuota.bUploadBypassed = TRUE;
		else if (m_SlQuota.nBytesAllowedToUl != -1)
		{
			if (nLimit == -1 || nLimit > (m_SlQuota.nBytesAllowedToUl - m_SlQuota.nUploaded))
				nLimit = m_SlQuota.nBytesAllowedToUl - m_SlQuota.nUploaded;
		}

		if (!nLimit)
			m_SlQuota.bContinueUpload = TRUE;
		
		return nLimit;
	}
	else 
	{
		CUser user;
		int nLimit = -1;
		if (m_status.loggedon && m_pOwner->m_pPermissions->GetUser(m_status.user, user))
		{
			nLimit = user.GetCurrentDownloadSpeedLimit();
		}
		if (nLimit > 0)
		{
			nLimit *= 100;
			if (m_SlQuota.nDownloaded >= nLimit)
			{
				m_SlQuota.bContinueDownload = TRUE;
				return 0;
			}
			else
				nLimit -= m_SlQuota.nDownloaded;
		}
		else
			nLimit = -1;
		if (user.BypassServerDownloadSpeedLimit())
			m_SlQuota.bDownloadBypassed = TRUE;
		else if (m_SlQuota.nBytesAllowedToDl != -1)
		{
			if (nLimit == -1 || nLimit > (m_SlQuota.nBytesAllowedToDl - m_SlQuota.nDownloaded))
				nLimit = m_SlQuota.nBytesAllowedToDl - m_SlQuota.nDownloaded;
		}
		
		if (!nLimit)
			m_SlQuota.bContinueDownload = TRUE;
		
		return nLimit;
	}
}

CControlSocket::CreateTransferSocket(CTransferSocket *pTransferSocket)
{
	/* Create socket
	 * First try control connection port - 1, if that fails try
	 * control connection port + 1. If that fails as well, let 
	 * the OS decide.
	 */
	BOOL bCreated = FALSE;
	// Get local port
	SOCKADDR_IN addr;
	int len = sizeof(addr);
	if (GetSockName((SOCKADDR *)&addr, &len))
	{
		int nPort = ntohs(addr.sin_port);
		// Try create control conn. port - 1
		if (nPort > 1)
			if (pTransferSocket->Create(nPort - 1, SOCK_STREAM, FD_CONNECT))
				bCreated = TRUE;
			// Try create control conn. port + 1 if necessary
			if (!bCreated && nPort < 65535)
				if (pTransferSocket->Create(nPort + 1, SOCK_STREAM, FD_CONNECT))
					bCreated = TRUE;
	}
	if (!bCreated)
		// Let the OS find a valid port
		if (!pTransferSocket->Create(0, SOCK_STREAM, FD_CONNECT))
		{
			// Give up
			Send("421 Can't create socket");
			ResetTransferstatus();
			return FALSE;
		}			
#ifndef NOLAYERS
	if (m_pGssLayer && m_pGssLayer->AuthSuccessful())
		m_transferstatus.socket->UseGSS(m_pGssLayer);
#endif		
	if (pTransferSocket->Connect(m_transferstatus.ip,m_transferstatus.port)==0)
	{
		if (GetLastError() != WSAEWOULDBLOCK)
		{
			Send("425 Can't open data connection");
			ResetTransferstatus();
			return FALSE;
		}
	}

	return TRUE;
}
