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

#if !defined(AFX_TRANSFERSOCKET_H__38ADA982_DD96_4607_B7D2_982011F162FE__INCLUDED_)
#define AFX_TRANSFERSOCKET_H__38ADA982_DD96_4607_B7D2_982011F162FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TransferSocket.h : Header-Datei
//

class CControlSocket;

#define TRANSFERMODE_NOTSET 0
#define TRANSFERMODE_LIST 1
#define TRANSFERMODE_RECEIVE 2
#define TRANSFERMODE_SEND 3
#define TRANSFERMODE_NLST 4

struct t_dirlisting;

/////////////////////////////////////////////////////////////////////////////
// Befehlsziel CTransferSocket 
#ifndef NOLAYERS
class CAsyncGssSocketLayer;
#endif

class CTransferSocket : public CAsyncSocketEx
{
// Attribute
public:

// Operationen
public:
	CTransferSocket(CControlSocket *pOwner);
	void Init(t_dirlisting *pDir, int nMode);
	void Init(CStdString filename, int nMode, _int64 rest, BOOL bBinary=TRUE);
#ifndef NOLAYERS
	void UseGSS(CAsyncGssSocketLayer *pGssLayer);
#endif
	virtual ~CTransferSocket();

// Überschreibungen
public:
	int GetMode() const;
	BOOL Started() const;
	BOOL CheckForTimeout();
	void PasvTransfer();
	int GetStatus();

	BOOL pasv;
// Implementierung
#if defined(_XBOX)
  public:
#else
  protected:
#endif
	virtual void OnSend(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnAccept(int nErrorCode);
	virtual void OnReceive(int nErrorCode);

#ifndef NOLAYERS
	virtual int OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nParam1, int nParam2);
#endif

	t_dirlisting *m_pDirListing;
	BOOL m_bSentClose;
	CStdString m_Filename;
	BOOL m_bReady;
	BOOL m_bStarted;
	BOOL InitTransfer(BOOL bCalledFromSend);
	int m_nMode;
	int m_status;
	CControlSocket *m_pOwner;
	_int64 m_nRest;
	BOOL m_bBinary;
	HANDLE m_hFile;
	char *m_pBuffer;
	int m_nBufferPos;
	BOOL bAccepted;
	SYSTEMTIME m_LastActiveTime;
#ifndef NOLAYERS
	CAsyncGssSocketLayer *m_pGssLayer;
#endif
	int m_nBufSize;
#if defined(_XBOX)
	int m_nPreAlloc;
	int m_nAlign;
#endif
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_TRANSFERSOCKET_H__38ADA982_DD96_4607_B7D2_982011F162FE__INCLUDED_
