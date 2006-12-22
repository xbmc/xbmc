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

#if !defined(AFX_CONTROLSOCKET_H__17DD46FD_8A4A_4394_9F90_C14BA65F6BF6__INCLUDED_)
#define AFX_CONTROLSOCKET_H__17DD46FD_8A4A_4394_9F90_C14BA65F6BF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ControlSocket.h : Header-Datei
//
#if defined(_XBOX)
  #include "bsdsfv.h"
#endif
class CAsyncGssSocketLayer;
class CTransferSocket;
/////////////////////////////////////////////////////////////////////////////
// Befehlsziel CControlSocket 

class CControlSocket : public CAsyncSocketEx
{
// Attribute
public:

// Operationen
public:
	CControlSocket(CServerThread *pOwner);
	virtual ~CControlSocket();

// Überschreibungen
public:
	CServerThread * m_pOwner;
	CStdString m_RemoteIP;
	void WaitGoOffline();
	BOOL m_bWaitGoOffline;
	void CheckForTimeout();
	void ForceClose(int nReason);
	CTransferSocket* GetTransferSocket();
	void ProcessTransferMsg();
	void ParseCommand();
	int m_userid;
	BOOL Send(LPCTSTR str);
	void SendStatus(LPCTSTR status,int type);
	BOOL GetCommand(CStdString &command,CStdString &args);
#if defined(_XBOX)
	BOOL SendCurDir(const CStdString command,CStdString curDir);
	BOOL SendDir(const CStdString command,CStdString curDir,const CStdString prompt);
	BOOL GetCommandFromString(const CStdString& source, CStdString &command,CStdString &args);
#endif
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	void Continue();
	
// Implementierung
protected:
	BOOL DoUserLogin(char* sendme);
	BOOL UnquoteArgs(CStdString &args);
	static int GetUserCount(const CStdString &user);
	static void IncUserCount(const CStdString &user);
	static void DecUserCount(const CStdString &user);
	void ResetTransferstatus();
	BOOL CreateTransferSocket(CTransferSocket *pTransferSocket);

	CAsyncGssSocketLayer *m_pGssLayer;

	std::list<CStdString> m_RecvLineBuffer;
	char m_RecvBuffer[2048];
	int m_nRecvBufferPos;
	char *m_pSendBuffer;
	int m_nSendBufferLen;

	int m_nTelnetSkip;
	BOOL m_bQuitCommand;
	SYSTEMTIME m_LastCmdTime, m_LastTransferTime, m_LoginTime;
	static std::map<CStdString, int> m_UserCount;
	CStdString m_CurrentDir;
	static CCriticalSectionWrapper m_Sync;
	struct t_status
	{
		BOOL loggedon;
		CStdString user;
		CStdString ip;
	} m_status;
	struct t_transferstatus
	{
		int pasv;
		_int64 rest;
		int type;
		CStdString ip;
		int port;
		CTransferSocket *socket;
	} m_transferstatus;
		
	CStdString RenName;
	BOOL bRenFile;

#if defined(_XBOX)  
  CSfvFile mSfvFile;
#endif

public:
	int GetSpeedLimit(int nMode);
	typedef struct {
		BOOL bContinueDownload;
		BOOL bContinueUpload;
		int nBytesAllowedToDl;
		int nBytesAllowedToUl;
		int nUploaded;
		int nDownloaded;		
		BOOL bDownloadBypassed;
		BOOL bUploadBypassed;
	} t_Quota;
	t_Quota m_SlQuota;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_CONTROLSOCKET_H__17DD46FD_8A4A_4394_9F90_C14BA65F6BF6__INCLUDED_
