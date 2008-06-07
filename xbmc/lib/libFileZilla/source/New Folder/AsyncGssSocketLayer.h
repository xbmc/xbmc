// GSSAsyncSocksifiedSocket.h: interface for the CAsyncGssSocketLayer CAsyncSocketEx.
//
//////////////////////////////////////////////////////////////////////
// Part of this code is copyright 2001 Massachusetts Institute of Technology

#if !defined(ASYNCGSSSOCKETLAYER_H__84779FB7_FC01_4743_996B_383E4D7045B7__INCLUDED_)
#define ASYNCGSSSOCKETLAYER_H__84779FB7_FC01_4743_996B_383E4D7045B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// **************************************************************************************
// File:            globals.h 
// By:              Arthur David Leather
// Created:         08/02/99
// Copyright        @1999 Massachusetts Institute of Technology - All rights reserved.
// Description:     H file for globals.cpp. Contains variables and functions 
//                  for SoFTP
//
// History:
//
// MM/DD/YY   Inits   Description of Change
// 08/02/99   ADL     Original
// **************************************************************************************


//#include "tlhelp32.h"
//#include <winsock.h>
#include "AsyncSocketExLayer.h"

#define GSS_INFO				0
#define GSS_ERROR				1
#define GSS_SHUTDOWN_COMPLETE	2
#define GSS_COMMAND				3
#define GSS_REPLY				4
#define GSS_AUTHFAILED			5
#define GSS_AUTHCOMPLETE		6

// Defines
#define GFTPDLL                             _T("FzGss.dll")
#define GSSAPI_AUTHENTICATION_SUCCEEDED     1

class CGssHelperWindow;
class CAsyncGssSocketLayer : public CAsyncSocketExLayer
{
public:
	CAsyncGssSocketLayer();
	virtual ~CAsyncGssSocketLayer();

	BOOL InitTransferChannel(CAsyncGssSocketLayer *pSocket);

	BOOL InitGSS(BOOL bSpawned = FALSE, BOOL promptPassword = FALSE);
	int GetClientAuth(const char* pHost);
	int ProcessCommand(const char *command, const char *args, char *sendme);
	int ProcessCommand(const char *command, const char *args1, const char *args2, char *sendme);
	BOOL AuthSuccessful() const;
	void SetAuthState(int i) {m_gotAuth = i;}
	void SetTransfer(BOOL flag) {m_transfer = flag;}
	BOOL ShutDownComplete();
	BOOL GetUserFromKrbTicket(char *buffer); //Should be 256 chars long

protected:
	void ReceiveReply();
	static int CALLBACK Callback(void *pData, int nParam1, int nParam2, int nParam3);
	virtual void Close();
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual BOOL ShutDown(int nHow /*=sends*/);

	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	
	char *m_pSendBuffer;
	int m_nSendBufferLen;
	int m_nSendBufferSize;

	char *m_pReceiveBuffer;
	int m_nReceiveBufferLen;
	int m_nReceiveBufferSize;
	char *m_pDecryptedReceiveBuffer;
	int m_nDecryptedReceiveBufferLen;
	int m_nDecryptedReceiveBufferSize;

	char m_tmpBuffer[1024*32]; //32KB buffer for temporary data

	BOOL m_bInitialized;

	BOOL m_nAwaitingReply; //Used by client authentication, set to true if waiting for a reply from the server
	BOOL m_transfer;

	int m_nShutDown;

	BOOL LoadGSSLibrary();
	BOOL UnLoadGSSLibrary();
	
	HINSTANCE m_hGSS_API;
	void **m_pData;
	BOOL m_bUseGSS;
	int m_gotAuth;  // authorization type

	// GSS-API
	typedef int (* t_FzGss_ProcessCommand)(void* m_pData, const char* command, char* args, char* sendme);
	typedef int (* t_FzGss_DecryptMessage)(void*, char *msgStr, char* sendme);
	typedef int (* t_FzGss_EncryptMessage)(void*, char *msgStr, char* sendme);
	typedef unsigned long (* t_FzGss_EncryptData)(void*, char *chunk, int length, char* sendme);
	typedef unsigned long (* t_FzGss_DecryptData)(void *pData, char *chunk, int length, char* send);
	typedef BOOL (* t_FzGss_InitGSS)(void*, int (CALLBACK *)(void *, int, int, int), void *, int);
	typedef BOOL (* t_FzGss_KillGSS)(void*);
	typedef int  (* t_FzGss_DoClientAuth)(void *pData, char *hostname, unsigned long myaddr, unsigned long hisaddr, char protLevel, int gssBufferSize);
	typedef int  (* t_FzGss_ProcessReply)(void *pData, char *reply);
	typedef int  (* t_FzGss_GetUserFromKrbTicket)(void *pData, char *buffer);

	t_FzGss_ProcessCommand pFzGss_ProcessCommand;
	t_FzGss_DecryptMessage pFzGss_DecryptMessage;
	t_FzGss_EncryptMessage pFzGss_EncryptMessage;
	t_FzGss_EncryptData pFzGss_EncryptData;
	t_FzGss_DecryptData pFzGss_DecryptData;
	t_FzGss_InitGSS pFzGss_InitGSS;
	t_FzGss_KillGSS pFzGss_KillGSS;
	t_FzGss_DoClientAuth pFzGss_DoClientAuth;
	t_FzGss_ProcessReply pFzGss_ProcessReply;
	t_FzGss_GetUserFromKrbTicket pFzGss_GetUserFromKrbTicket;

	BOOL KillGSSData();

private:
	int m_nGssNetworkError;
};


#endif // !defined(ASYNCGSSSOCKETLAYER_H__84779FB7_FC01_4743_996B_383E4D7045B7__INCLUDED_)
