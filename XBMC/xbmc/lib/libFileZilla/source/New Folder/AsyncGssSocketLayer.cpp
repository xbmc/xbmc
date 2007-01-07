// GSSAsyncSocksifiedSocket.cpp: implementation of the CAsyncGssSocketLayer class.
//
//////////////////////////////////////////////////////////////////////
// Part of this code is copyright 2001 Massachusetts Institute of Technology

#include "stdafx.h"
#include "resource.h"
#include "AsyncGssSocketLayer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define BUFSIZE (1024*8)

CAsyncGssSocketLayer::CAsyncGssSocketLayer()
{
	m_hGSS_API = NULL;
	m_transfer = FALSE;
	m_bInitialized=FALSE;
	m_bUseGSS=FALSE;
	m_nGssNetworkError=0;
	m_gotAuth=0;
	m_nShutDown = 0;

	pFzGss_ProcessCommand = NULL;
	pFzGss_InitGSS = NULL;
	pFzGss_KillGSS = NULL;
	pFzGss_DecryptMessage = NULL;
	pFzGss_EncryptMessage = NULL;
	pFzGss_EncryptData = NULL;
	pFzGss_DecryptData = NULL;

	m_pSendBuffer = NULL;
	m_nSendBufferLen = 0;
	m_nSendBufferSize = 0;

	m_pReceiveBuffer = NULL;
	m_nReceiveBufferLen = 0;
	m_nReceiveBufferSize = 0;

	m_pDecryptedReceiveBuffer = NULL;
	m_nDecryptedReceiveBufferLen = 0;
	m_nDecryptedReceiveBufferSize = 0;

	m_nAwaitingReply = 0;
}

CAsyncGssSocketLayer::~CAsyncGssSocketLayer()
{
	delete [] m_pSendBuffer;
	delete [] m_pReceiveBuffer;
	delete [] m_pDecryptedReceiveBuffer;
	
	KillGSSData();
}

int CAsyncGssSocketLayer::Send(const void* lpBuf, int nBufLen, int nFlags)
{
	if (m_gotAuth == GSSAPI_AUTHENTICATION_SUCCEEDED && m_bUseGSS)
	{
		if (m_nAwaitingReply == 3)
		{
			m_nAwaitingReply = 0;
			int res = SendNext(lpBuf, nBufLen, nFlags);
			if (res == nBufLen)
				m_nAwaitingReply = 0;
			return res;
		}
		if (m_nShutDown)
		{
			SetLastError(WSAESHUTDOWN);
			return SOCKET_ERROR;
		}
		
		if (!nBufLen)
			return 0;

		if (m_nSendBufferLen > BUFSIZE)
		{
			SetLastError(WSAEWOULDBLOCK);
			return SOCKET_ERROR;
		}

		char sendme[4096];
		char *encBuffer = m_tmpBuffer;
		int encBufferLen = 0;

		if ((nBufLen*1.5) > (1024*32))
			encBuffer = new char[(int)(nBufLen * 1.5)];
		memcpy(encBuffer, (char*)lpBuf, nBufLen);

		if (m_transfer)
		{
			if (nBufLen>4000)
				nBufLen=4000;
			
			encBuffer[nBufLen] = '\0';
			encBufferLen = pFzGss_EncryptData(m_pData, encBuffer, nBufLen, sendme);
		
			if (!encBufferLen)
			{
				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;
				return 0;
			}
		}
		else
		{
			encBuffer[nBufLen]='\0';
			int len = pFzGss_EncryptMessage(m_pData, encBuffer, sendme);
			if (!len) 
			{
				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;
				return 0;
			}

			char *str = new char[strlen(encBuffer) + 30];
			sprintf(str, "Encrypted command: %s", encBuffer);
			DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_INFO, (int)str);
			delete [] str;
			strcat(encBuffer, "\r\n");
			encBufferLen = strlen(encBuffer);
		}

		if (m_nSendBufferLen)
		{
			ASSERT(m_nSendBufferLen <= m_nSendBufferSize);
			ASSERT(m_pSendBuffer);
			int numsent = SendNext(m_pSendBuffer, m_nSendBufferLen, 0);
			if (!numsent)
			{
				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;
				return 0;
			}
			else if (numsent == SOCKET_ERROR && GetLastError()!=WSAEWOULDBLOCK)
			{
				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;
				return SOCKET_ERROR;
			}
			else if (numsent != m_nSendBufferLen)
			{
				if (numsent == SOCKET_ERROR)
					numsent = 0;
				if (!m_pSendBuffer)
				{
					m_pSendBuffer = new char[encBufferLen * 2];
					m_nSendBufferSize = encBufferLen * 2;
				}
				else if (m_nSendBufferSize < (m_nSendBufferLen + encBufferLen))
				{
					char *tmp = m_pSendBuffer;
					m_pSendBuffer = new char[m_nSendBufferSize + encBufferLen + 4096];
					m_nSendBufferSize = m_nSendBufferSize + encBufferLen + 4096;
					memcpy(m_pSendBuffer, tmp+numsent, m_nSendBufferLen-numsent);
					delete [] tmp;
				}
				else
					memmove(m_pSendBuffer, m_pSendBuffer + numsent, m_nSendBufferLen - numsent);
				memcpy(m_pSendBuffer + m_nSendBufferLen - numsent, encBuffer, encBufferLen);
				m_nSendBufferLen += encBufferLen-numsent;

				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;

				return nBufLen;
			}
			else
				m_nSendBufferLen = 0;
		}

		int numsent = SendNext(encBuffer, encBufferLen, 0);
		if (!numsent)
		{
			if (encBuffer != m_tmpBuffer)
				delete [] encBuffer;
			return 0;
		}
		if (numsent == SOCKET_ERROR)
		{
			if (GetLastError() != WSAEWOULDBLOCK)
			{
				if (encBuffer != m_tmpBuffer)
					delete [] encBuffer;
				return SOCKET_ERROR;
			}
			else
			{
				if (m_nSendBufferSize < encBufferLen)
				{
					delete [] m_pSendBuffer;
					m_pSendBuffer = new char[encBufferLen * 2];
					m_nSendBufferSize = encBufferLen * 2;
				}

				memcpy(m_pSendBuffer, encBuffer, encBufferLen);
				m_nSendBufferLen = encBufferLen;
			}
		}
		else if (numsent != encBufferLen)
		{
			if (m_nSendBufferSize < encBufferLen)
			{
				delete [] m_pSendBuffer;
				m_pSendBuffer = new char[encBufferLen * 2];
				m_nSendBufferSize = encBufferLen * 2;
			}
			memcpy(m_pSendBuffer, encBuffer+numsent, encBufferLen-numsent);
			m_nSendBufferLen = encBufferLen-numsent;
		}

		if (encBuffer != m_tmpBuffer)
			delete [] encBuffer;
		
		return nBufLen;
	}
	else
		return SendNext(lpBuf, nBufLen, nFlags);
}
/*
This method calls its super method to receive data.
If authentication succeeded at some point, then it tries to decrypt the data.
After decryption, it adds \r\n to the end of the buffer.
This method currently does not handle the MSG_PEEK flag, it also ignores nBufLen.
*/
int CAsyncGssSocketLayer::Receive(void *lpBuf, int nBufLen, int nFlags)
{
	if (m_gotAuth == GSSAPI_AUTHENTICATION_SUCCEEDED && m_bUseGSS)
	{
		if (m_nShutDown)
		{
			SetLastError(WSAESHUTDOWN);
			return SOCKET_ERROR;
		}

		if (!nBufLen)
			return 0;
	
		BOOL bTriggerRead = TRUE;
		if (!m_nDecryptedReceiveBufferLen)
		{
			bTriggerRead = FALSE;
			OnReceive(0);
		}
		
		if (m_nDecryptedReceiveBufferLen)
		{
			ASSERT(m_pDecryptedReceiveBuffer && m_nDecryptedReceiveBufferLen<=m_nDecryptedReceiveBufferSize);
			if (m_nDecryptedReceiveBufferLen > nBufLen)
			{
				memcpy(lpBuf, m_pDecryptedReceiveBuffer, nBufLen);
				memmove(m_pDecryptedReceiveBuffer, m_pDecryptedReceiveBuffer + nBufLen, m_nDecryptedReceiveBufferLen-nBufLen);
				m_nDecryptedReceiveBufferLen -= nBufLen;
				ASSERT(nBufLen>0);

				if (bTriggerRead)
					TriggerEvent(FD_READ, 0);

				return nBufLen;
			}
			else if (m_nDecryptedReceiveBufferLen == nBufLen)
			{
				memcpy(lpBuf, m_pDecryptedReceiveBuffer, nBufLen);
				m_nDecryptedReceiveBufferLen = 0;

				if (m_nGssNetworkError == -1)
				{
					//Trigger OnClose()
					TriggerEvent(FD_CLOSE, 0, TRUE);
				}
				else if (bTriggerRead)
					TriggerEvent(FD_READ, 0);

				return nBufLen;
			}
			else
			{
				memcpy(lpBuf, m_pDecryptedReceiveBuffer, m_nDecryptedReceiveBufferLen);
				int res = m_nDecryptedReceiveBufferLen;
				m_nDecryptedReceiveBufferLen = 0;

				if (m_nGssNetworkError == -1)
				{
					//Trigger OnClose()
					TriggerEvent(FD_CLOSE, 0, TRUE);
				}
				else if (bTriggerRead)
					TriggerEvent(FD_READ, 0);
				
				return res;
			}
		}
		else
		{
			if (m_nGssNetworkError==-1)
				return 0;
			else if (m_nGssNetworkError)
			{
				WSASetLastError(m_nGssNetworkError);
				return SOCKET_ERROR;
			}
			else
			{
				WSASetLastError(WSAEWOULDBLOCK);
				return SOCKET_ERROR;
			}
		}
	}
	else
		return ReceiveNext(lpBuf, nBufLen, nFlags);
}

void CAsyncGssSocketLayer::OnReceive(int nErrorCode) 
{
	if (!m_bUseGSS || m_gotAuth != GSSAPI_AUTHENTICATION_SUCCEEDED)
	{
		TriggerEvent(FD_READ, nErrorCode, TRUE);
		return;
	}

	//Don't decrypt additional data if buffer for decrypted data is not empty
	if (m_nDecryptedReceiveBufferLen && (GetLayerState()==attached || GetLayerState()==connected))
	{
		TriggerEvent(FD_READ, nErrorCode, TRUE);
		return;
	}

	char sendme[4096];
	//TRACE(m_transfer?"GSS OnReceive: called, transfer mode\n":"GSS OnReceive: called\n");
	int count = 10;
	while(count--)
	{
		if (m_transfer)
		{
			if (m_nReceiveBufferLen < 4)
			{
				if (!m_pReceiveBuffer)
				{
					ASSERT(!m_nReceiveBufferLen);
					ASSERT(!m_nReceiveBufferSize);
					m_pReceiveBuffer = new char[4096];
					m_nReceiveBufferSize = 4096;
					m_nReceiveBufferLen = 0;
				}
				int numread = ReceiveNext(m_pReceiveBuffer+m_nReceiveBufferLen, 4 - m_nReceiveBufferLen);
				if (!numread)
				{
					m_nGssNetworkError = -1;
					if (!m_nDecryptedReceiveBufferLen)
						TriggerEvent(FD_CLOSE, 0, TRUE);
					return;
				}
				else if (numread == SOCKET_ERROR)
				{
					if (GetLastError() != WSAEWOULDBLOCK)
					{
						TriggerEvent(FD_CLOSE, 0, TRUE);
						m_nGssNetworkError = GetLastError();
					}
					else
						if (m_nDecryptedReceiveBufferLen)
							TriggerEvent(FD_READ, nErrorCode, TRUE);
					return;
				}
				m_nReceiveBufferLen += numread;
				
				if (m_nReceiveBufferLen != 4)
				{
					if (m_nDecryptedReceiveBufferLen)
						TriggerEvent(FD_READ, nErrorCode, TRUE);
					return;
				}

				char tmp = m_pReceiveBuffer[0];
				m_pReceiveBuffer[0] = m_pReceiveBuffer[3];
				m_pReceiveBuffer[3] = tmp;
				tmp = m_pReceiveBuffer[1];
				m_pReceiveBuffer[1] = m_pReceiveBuffer[2];
				m_pReceiveBuffer[2] = tmp;
				unsigned int len = *(unsigned int*)m_pReceiveBuffer;
				if (len<4 || len > (1024*1024*4))
				{
					m_nGssNetworkError = WSAEMSGSIZE;
					TriggerEvent(FD_CLOSE, 0, TRUE);
					return;
				}
					
				if (m_nReceiveBufferSize < (len+4))
				{
					delete [] m_pReceiveBuffer;
					m_pReceiveBuffer = new char[len * 2 + 4];
					m_nReceiveBufferSize = len * 2 + 4;
				}
				memcpy(m_pReceiveBuffer, &len, 4);
				m_nReceiveBufferLen = 4;
			}

			ASSERT(m_pReceiveBuffer);
			int len = *(int*)m_pReceiveBuffer;
			ASSERT(len>4 && len < (1024*1024*4));
			int lenToReceive = len - m_nReceiveBufferLen + 4;

			int numread = ReceiveNext(m_pReceiveBuffer + m_nReceiveBufferLen, lenToReceive);

			if (!numread)
			{
				m_nGssNetworkError = -1;
				if (!m_nDecryptedReceiveBufferLen)
					TriggerEvent(FD_CLOSE, 0, TRUE);
				return;
			}
			else if (numread == SOCKET_ERROR)
			{
				if (GetLastError() != WSAEWOULDBLOCK)
				{
					m_nGssNetworkError = GetLastError();
					TriggerEvent(FD_CLOSE, 0, TRUE);
				}
				return;
			}
			m_nReceiveBufferLen += numread;
				
			if (numread != lenToReceive)
			{
				if (m_nDecryptedReceiveBufferLen)
					TriggerEvent(FD_READ, nErrorCode, TRUE);
				return;
			}

			ASSERT(m_nReceiveBufferLen-4 == len);

			char * decBuffer = m_tmpBuffer;
			if ((len*1.5) > (1024*32))
				decBuffer = new char[(int)(len * 1.5)];
			memcpy(decBuffer, m_pReceiveBuffer+4, len);

			m_nReceiveBufferLen = 0;

			int nDecrypted = pFzGss_DecryptData(m_pData, decBuffer, len, sendme);
			if (nDecrypted <= 0)
			{
				if (!m_nDecryptedReceiveBufferLen)
					//Trigger OnClose()
					TriggerEvent(FD_CLOSE, 0, TRUE);
				
				if (decBuffer != m_tmpBuffer)
					delete [] decBuffer;
				return;
			}

			//Add line to decrypted buffer
			if (!m_pDecryptedReceiveBuffer)
			{
				ASSERT(!m_nDecryptedReceiveBufferSize && !m_nDecryptedReceiveBufferLen);
				m_pDecryptedReceiveBuffer = new char[nDecrypted * 2];
				m_nDecryptedReceiveBufferSize = nDecrypted * 2;
				m_nDecryptedReceiveBufferLen = 0;
			}
			else if (m_nDecryptedReceiveBufferSize < (m_nDecryptedReceiveBufferLen+nDecrypted))
			{
				char *tmp=m_pDecryptedReceiveBuffer;
				m_pDecryptedReceiveBuffer = new char[m_nDecryptedReceiveBufferSize + nDecrypted + 4096];
				m_nDecryptedReceiveBufferSize = m_nDecryptedReceiveBufferSize + nDecrypted + 4096;
				memcpy(m_pDecryptedReceiveBuffer, tmp, m_nDecryptedReceiveBufferLen);
				delete [] tmp;
			}
			memcpy(m_pDecryptedReceiveBuffer + m_nDecryptedReceiveBufferLen, decBuffer, nDecrypted);
			m_nDecryptedReceiveBufferLen += nDecrypted;
				
			if (decBuffer != m_tmpBuffer)
				delete [] decBuffer;
		}
		else 
		{
			if (m_nAwaitingReply == 1)
			{
				ReceiveReply();
				return;
			}
			int numread = ReceiveNext(m_tmpBuffer, BUFSIZE);
			if (!numread)
			{
				m_nGssNetworkError = -1;

				//Trigger OnClose()
				TriggerEvent(FD_CLOSE, 0, TRUE);
				return;
			}
			else if ( numread == SOCKET_ERROR )
			{	
				if ( WSAGetLastError() != WSAEWOULDBLOCK )
				{
					m_nGssNetworkError = GetLastError();
					TriggerEvent(FD_CLOSE, 0, TRUE);
				}
				else if (m_nDecryptedReceiveBufferLen)
					TriggerEvent(FD_READ, 0, TRUE);
				return;
			}
			if (!m_pReceiveBuffer)
			{
				ASSERT(!m_nReceiveBufferLen && !m_nReceiveBufferSize);
				m_pReceiveBuffer = new char[numread * 2];
				m_nReceiveBufferSize = numread * 2;
			}
			else if (m_nReceiveBufferSize < (m_nReceiveBufferLen + numread))
			{
				char *tmp=m_pReceiveBuffer;
				m_pReceiveBuffer = new char[m_nReceiveBufferLen + numread + 4096];
				m_nReceiveBufferSize = m_nReceiveBufferLen + numread + 4096;
				memcpy(m_pReceiveBuffer, tmp, m_nReceiveBufferLen);
				delete [] tmp;
			}
			memcpy(m_pReceiveBuffer+m_nReceiveBufferLen, m_tmpBuffer, numread);
			int nOldLen = m_nReceiveBufferLen;
			m_nReceiveBufferLen += numread;

			//Now search for complete lines and decrypt them
			for (int i=nOldLen; i<m_nReceiveBufferLen; i++)
			{
				if (m_pReceiveBuffer[i]=='\n')
				{
					if (!i)
					{
						if (m_nReceiveBufferLen>1)
							memmove(m_pReceiveBuffer, m_pReceiveBuffer+1, m_nReceiveBufferLen-1);
						m_nReceiveBufferLen--;
						i--;
						continue;
					}
					if (m_pReceiveBuffer[i-1]=='\r')
					{
						//We've found a line

						char *decBuffer = m_tmpBuffer;
						if ((i*1.5) > (1024*32))
							decBuffer = new char[(int)(i * 1.5)];
						memcpy(decBuffer, m_pReceiveBuffer, i-2);
						decBuffer[i-1] = '\0';

						//Delete encrypted line from receive buffer
						if (!(m_nReceiveBufferLen-i-1))
							m_nReceiveBufferLen = 0;
						else
						{
							memmove(m_pReceiveBuffer, m_pReceiveBuffer+i+1, m_nReceiveBufferLen - i - 1);
							m_nReceiveBufferLen -= i + 1;
						}

						// Decrypt line
						char *str = new char[strlen(decBuffer) + 30];
						sprintf(str, "Encrypted reply: %s", decBuffer);
						DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_INFO, (int)str);
						delete [] str;
						int nDecrypted = pFzGss_DecryptMessage(m_pData, decBuffer, sendme);
						if (!nDecrypted)
						{
							m_nGssNetworkError = -1;
							
							//Trigger OnClose()
							TriggerEvent(FD_CLOSE, 0, TRUE);
							
							if (decBuffer != m_tmpBuffer)
								delete [] decBuffer;
							return;
						}
						nDecrypted = strlen(decBuffer);
					
						if (m_nAwaitingReply && m_nAwaitingReply<3)
						{
							while (decBuffer[nDecrypted - 1]=='\n' || decBuffer[nDecrypted - 1]=='\r')
								nDecrypted--;
							decBuffer[nDecrypted] = 0;
							m_nAwaitingReply = 0;
							DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_REPLY, (int)decBuffer);
							if (decBuffer[nDecrypted - 1]!='\n')
							{
								decBuffer[nDecrypted++]='\r';
								decBuffer[nDecrypted++]='\n';
								decBuffer[nDecrypted] = 0;
							}

							int res = pFzGss_ProcessReply(m_pData, decBuffer);
							if (res == 1)
								DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_AUTHCOMPLETE, 0);
							else if (res!=-1)
							{
								m_gotAuth = 0;
								DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_AUTHFAILED, 0);
							}
							if (decBuffer != m_tmpBuffer)
								delete [] decBuffer;
						}
						else
						{
							if (decBuffer[nDecrypted - 1]!='\n')
							{
								decBuffer[nDecrypted++]='\r';
								decBuffer[nDecrypted++]='\n';
								decBuffer[nDecrypted] = 0;
							}
							
							//Add line to decrypted buffer
							if (!m_pDecryptedReceiveBuffer)
							{
								m_pDecryptedReceiveBuffer = new char[nDecrypted * 2];
								m_nDecryptedReceiveBufferSize = nDecrypted * 2;
								m_nDecryptedReceiveBufferLen=0;
							}
							else if (m_nDecryptedReceiveBufferSize < (m_nDecryptedReceiveBufferLen+nDecrypted))
							{
								char *tmp=m_pDecryptedReceiveBuffer;
								m_pDecryptedReceiveBuffer = new char[m_nDecryptedReceiveBufferSize + nDecrypted + 4096];
								m_nDecryptedReceiveBufferSize = m_nDecryptedReceiveBufferSize + nDecrypted + 4096;
								memcpy(m_pDecryptedReceiveBuffer, tmp, m_nDecryptedReceiveBufferLen);
								delete [] tmp;
							}
							memcpy(m_pDecryptedReceiveBuffer + m_nDecryptedReceiveBufferLen, decBuffer, nDecrypted);
							m_nDecryptedReceiveBufferLen += nDecrypted;
	
							if (decBuffer != m_tmpBuffer)
								delete [] decBuffer;
						}
						
						//Try to decrypt any additional lines
						i=-1;
					}
				}
			}
		}
	}
}

void CAsyncGssSocketLayer::Close()
{
	CloseNext();
	
	m_nAwaitingReply = 0;
	m_nGssNetworkError=0;
	if (!m_transfer)
		m_gotAuth = 0;

	m_nSendBufferLen = 0;

	m_nReceiveBufferLen = 0;
	m_nDecryptedReceiveBufferLen = 0;

	KillGSSData();
}

BOOL CAsyncGssSocketLayer::UnLoadGSSLibrary()
{
	if (m_hGSS_API)
	{
		FreeLibrary(m_hGSS_API);
		m_hGSS_API = NULL;
	}
	return TRUE;
}


BOOL CAsyncGssSocketLayer::LoadGSSLibrary()
{
	if (m_hGSS_API)
		return TRUE;

	m_hGSS_API = LoadLibrary(GFTPDLL);
	return !(m_hGSS_API==NULL); 
	
}

BOOL CAsyncGssSocketLayer::InitGSS(BOOL bSpawned, BOOL promptPassword)
{
	if (m_bUseGSS)
		return TRUE;

	if (m_bInitialized)
		return TRUE;

	if (!m_hGSS_API)
		LoadGSSLibrary();
	
	if (!m_hGSS_API)
		return FALSE;
	
	if (m_hGSS_API)
	{
		pFzGss_ProcessCommand	= (t_FzGss_ProcessCommand)GetProcAddress(m_hGSS_API, "ProcessCommand");
		pFzGss_DecryptMessage	= (t_FzGss_DecryptMessage)GetProcAddress(m_hGSS_API, "DecryptMessage");
		pFzGss_EncryptMessage	= (t_FzGss_EncryptMessage)GetProcAddress(m_hGSS_API, "EncryptMessage");
		pFzGss_EncryptData		= (t_FzGss_EncryptData)GetProcAddress(m_hGSS_API , "EncryptData");
        pFzGss_DecryptData		= (t_FzGss_DecryptData)GetProcAddress(m_hGSS_API, "DecryptData");
		pFzGss_InitGSS			= (t_FzGss_InitGSS)GetProcAddress(m_hGSS_API, "InitGSS");
		pFzGss_KillGSS			= (t_FzGss_KillGSS)GetProcAddress(m_hGSS_API, "KillGSS");
		pFzGss_DoClientAuth		= (t_FzGss_DoClientAuth)GetProcAddress(m_hGSS_API, "DoClientAuth");
		pFzGss_ProcessReply		= (t_FzGss_ProcessReply)GetProcAddress(m_hGSS_API, "ProcessReply");
		pFzGss_GetUserFromKrbTicket = (t_FzGss_GetUserFromKrbTicket)GetProcAddress(m_hGSS_API, "GetUserFromKrbTicket");
		
		if (!pFzGss_ProcessCommand || !pFzGss_InitGSS || !pFzGss_KillGSS||
			!pFzGss_DecryptMessage || !pFzGss_EncryptMessage || !pFzGss_EncryptData ||
			!pFzGss_DecryptData		||
			!pFzGss_DoClientAuth	||
			!pFzGss_ProcessReply	||
			!pFzGss_GetUserFromKrbTicket
			)
		{
			return FALSE;
		}
	}

	if (!bSpawned)
	{
		m_pData=new void*;
		pFzGss_InitGSS(m_pData, Callback, this, promptPassword);
	}
	
	if (!bSpawned)
		m_bInitialized = TRUE;

	m_bUseGSS = TRUE;
	
	return TRUE;
}

BOOL CAsyncGssSocketLayer::KillGSSData()
{
	if (!m_bUseGSS)
		return FALSE;

	if (!m_bInitialized)
		return TRUE;

	m_bUseGSS = FALSE;

	pFzGss_KillGSS(m_pData);
	m_bInitialized = FALSE;
	delete m_pData;
	m_pData = NULL;
	
	UnLoadGSSLibrary();

	return TRUE;
}

BOOL CAsyncGssSocketLayer::InitTransferChannel(CAsyncGssSocketLayer *pSocket)
{
	KillGSSData();
	InitGSS(TRUE);
	m_pData = pSocket->m_pData;
	m_bUseGSS = pSocket->m_bUseGSS;
	m_gotAuth = pSocket->m_gotAuth;
	m_transfer = TRUE;
	return TRUE;
}

void CAsyncGssSocketLayer::OnSend(int nErrorCode)
{
	if (!m_nSendBufferLen)
		TriggerEvent(FD_WRITE, nErrorCode, TRUE);
	else
	{
		ASSERT(m_pSendBuffer);
		int numsent = SendNext(m_pSendBuffer, m_nSendBufferLen, 0);
		if (!numsent)
			TriggerEvent(FD_CLOSE, nErrorCode, TRUE);
		else if (numsent == SOCKET_ERROR)
		{
			if (GetLastError() != WSAEWOULDBLOCK)
				TriggerEvent(FD_CLOSE, nErrorCode, TRUE);
		}
		else if (numsent != m_nSendBufferLen)
		{
			memmove(m_pSendBuffer, m_pSendBuffer + numsent, m_nSendBufferLen - numsent);
			m_nSendBufferLen -= numsent;
		}
		else
		{
			m_nSendBufferLen = 0;
			if (ShutDownComplete())
				DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_SHUTDOWN_COMPLETE, 0);
			else
				TriggerEvent(FD_WRITE, nErrorCode, TRUE);
		}
	}
}

BOOL CAsyncGssSocketLayer::ShutDown(int nHow /*=sends*/)
{
	if (m_gotAuth == GSSAPI_AUTHENTICATION_SUCCEEDED && m_bUseGSS)
	{
		if (m_nShutDown)
		{
			if (m_nSendBufferLen)
				return FALSE;
			else
				return TRUE;
		}
		m_nShutDown = 1;

		char sendme[4096];
		char *encBuffer = m_tmpBuffer;
		int encBufferLen = 0;

		encBufferLen = pFzGss_EncryptData(m_pData, encBuffer, 0, sendme);
		if (!encBufferLen)
		{
			WSASetLastError(WSAENETDOWN);
			return FALSE;
		}
		
		encBuffer[encBufferLen++] = '\r';
		encBuffer[encBufferLen++] = '\n';

		if (m_nSendBufferLen)
		{
			ASSERT(m_pSendBuffer);
			int numsent = SendNext(m_pSendBuffer, m_nSendBufferLen, 0);
			if (!numsent)
				return TRUE;
			else if (numsent == SOCKET_ERROR && GetLastError()!=WSAEWOULDBLOCK)
				return TRUE;
			else if (numsent != m_nSendBufferLen)
			{
				if (numsent == SOCKET_ERROR)
					numsent = 0;
				if (m_nSendBufferSize < (m_nSendBufferLen + encBufferLen))
				{
					char *tmp = m_pSendBuffer;
					m_pSendBuffer = new char[m_nSendBufferSize + encBufferLen + 4096];
					m_nSendBufferSize = m_nSendBufferSize + encBufferLen + 4096;
					memcpy(m_pSendBuffer, tmp+numsent, m_nSendBufferLen-numsent);
					delete [] tmp;
				}
				else
					memmove(m_pSendBuffer, m_pSendBuffer + numsent, m_nSendBufferLen - numsent);
				memcpy(m_pSendBuffer + m_nSendBufferLen-numsent, encBuffer, encBufferLen);
				m_nSendBufferLen += encBufferLen-numsent;

				WSASetLastError(WSAEWOULDBLOCK);
				return FALSE;
			}
			else
				m_nSendBufferLen = 0;
		}

		int numsent = SendNext(encBuffer, encBufferLen, 0);
		if (!numsent)
			return TRUE;
		if (numsent == SOCKET_ERROR)
		{
			if (GetLastError() == WSAEWOULDBLOCK)
			{
				if (m_nSendBufferSize < encBufferLen)
				{
					delete [] m_pSendBuffer;
					m_pSendBuffer = new char[encBufferLen * 2];
					m_nSendBufferLen = encBufferLen * 2;
				}

				memcpy(m_pSendBuffer, encBuffer, encBufferLen);
				m_nSendBufferLen = encBufferLen;
				return FALSE;
			}
			return TRUE;
		}

		if (numsent != encBufferLen)
		{
			if (m_nSendBufferSize < encBufferLen)
			{
				delete [] m_pSendBuffer;
				m_pSendBuffer = new char[encBufferLen * 2];
				m_nSendBufferSize = encBufferLen * 2;
			}
			memcpy(m_pSendBuffer, encBuffer+numsent, encBufferLen-numsent);
			m_nSendBufferLen = encBufferLen-numsent;
		}

		if (m_nSendBufferLen)
		{
			WSASetLastError(WSAEWOULDBLOCK);
			return FALSE;
		}
		else
			return TRUE;
	}
	else
		return ShutDownNext();
}

BOOL CAsyncGssSocketLayer::ShutDownComplete()
{
	//If a ShutDown was issued, has the connection already been shut down?
	if (!m_nShutDown)
		return FALSE;
	else if (m_gotAuth != GSSAPI_AUTHENTICATION_SUCCEEDED || !m_bUseGSS)
		return FALSE;
	else if (m_nSendBufferLen)
		return FALSE;
	else
		return TRUE;
}

int CAsyncGssSocketLayer::ProcessCommand(const char *command, const char *args, char *sendme)
{
	ASSERT(command);
	ASSERT(sendme);

	if (!m_bUseGSS)
	{
		strcpy(sendme, "501 GSSAPI not initialized");
		return -1;
	}
	
	char *argBuffer = m_tmpBuffer;
	if ((strlen(args) + 1000) >= (1024*32))
		argBuffer = new char[strlen(args) + 1000];
	
	if (!strcmp(command, "ADAT"))
	{
		SOCKADDR_IN his_addr;
		SOCKADDR_IN ctrl_addr;
		
		int addrlen = sizeof (his_addr);
		if (!GetPeerName((SOCKADDR*)&his_addr, &addrlen))
		{
			if (argBuffer != m_tmpBuffer)
				delete [] argBuffer;
			sprintf(sendme, "501 unable to getpeername");
			return -1;
		}
		
		addrlen = sizeof (ctrl_addr);
		if (!GetSockName((SOCKADDR*)&ctrl_addr, &addrlen))
		{
			if (argBuffer != m_tmpBuffer)
				delete [] argBuffer;
			sprintf(sendme, "501 unable to getsockname");
			return -1;
		}
		
		char localname[513];
		struct hostent *hp;

		if (gethostname(localname, 512))
		{
			if (argBuffer != m_tmpBuffer)
				delete [] argBuffer;
			sprintf(sendme, "501 couldn't get local hostname (%d)", errno);
			return -1;
		}
		
		if (!(hp = gethostbyname(localname))) 
		{
			if (argBuffer != m_tmpBuffer)
				delete [] argBuffer;
			sprintf(sendme, "501 couldn't canonicalize local hostname");
			return -1;
		}
		strncpy(localname, hp->h_name, sizeof(localname) - 1);
		localname[sizeof(localname) - 1] = '\0';
		
		memcpy(argBuffer, &his_addr.sin_addr.s_addr, 4);
		memcpy(argBuffer+4, &ctrl_addr.sin_addr.s_addr, 4);
		strcpy(argBuffer + 8, localname);
		strcpy(argBuffer + 8 + strlen(localname) + 1, args);
	}
	else
		strcpy(argBuffer, args);

	int res = pFzGss_ProcessCommand(m_pData, command, argBuffer, sendme);

	if (!strcmp(command, "ADAT") && res != -1)
	{
		m_nAwaitingReply = 3;
		m_gotAuth = GSSAPI_AUTHENTICATION_SUCCEEDED;
	}
	
	if (argBuffer != m_tmpBuffer)
		delete [] argBuffer;

	return res;
}

BOOL CAsyncGssSocketLayer::AuthSuccessful() const
{
	return m_gotAuth == GSSAPI_AUTHENTICATION_SUCCEEDED;
}

int CAsyncGssSocketLayer::ProcessCommand(const char *command, const char *args1, const char *args2, char *sendme)
{
	ASSERT(command);
	ASSERT(sendme);

	if (!m_bUseGSS)
	{
		strcpy(sendme, "501 GSSAPI not initialized");
		return -1;
	}
	
	char *argBuffer = m_tmpBuffer;
	if ((strlen(args1) + strlen(args2) + 10) >= (1024*32))
		argBuffer = new char[strlen(args1) + strlen(args2) + 10];
	
	strcpy(argBuffer, args1);
	strcpy(argBuffer + strlen(args1) + 1, args2);

	int res = pFzGss_ProcessCommand(m_pData, command, argBuffer, sendme);

	if (!strcmp(command, "ADAT") && res != -1)
		m_gotAuth = GSSAPI_AUTHENTICATION_SUCCEEDED;
	
	if (argBuffer != m_tmpBuffer)
		delete [] argBuffer;

	return res;
}

int CAsyncGssSocketLayer::GetClientAuth(const char* pHost)
{
	if (m_hGSS_API)
	{
		char *hostname;
		ULONG peeraddr;
		ULONG myaddr;
		int res;
		
		hostname = new char[strlen(pHost)+1];
		strcpy(hostname, pHost);
	
		SOCKADDR SockAddr;
		memset(&SockAddr,0,sizeof(SockAddr));
		int SockAddrLen=sizeof(SockAddr);
		if (!GetPeerName(&SockAddr, &SockAddrLen))
		{
			DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_ERROR, (int)"GetAuth() GetPeerName() failed");
			return FALSE;
		}
		peeraddr=((LPSOCKADDR_IN)&SockAddr)->sin_addr.S_un.S_addr;
		
		memset(&SockAddr, 0, sizeof(SockAddr));
		if (!GetSockName(&SockAddr, &SockAddrLen))
		{
			DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_ERROR, (int)"GetAuth() GetSockName() failed");
			return FALSE;
		}
		myaddr=((LPSOCKADDR_IN)&SockAddr)->sin_addr.S_un.S_addr;
		
		res = pFzGss_DoClientAuth(m_pData, hostname,
			myaddr,
			peeraddr,
			'P', 0);
		
		m_gotAuth = res;
		if (m_gotAuth == -1)
			m_gotAuth = 1;
		delete [] hostname;
		return res;
	}
	else
	{
		DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_ERROR, (int)"GetClientAuth(933): GSS api not initialized!");
		m_gotAuth = 0;//SEND_DATA_IN_THE_CLEAR;
	}
	return m_gotAuth;
}

void CAsyncGssSocketLayer::OnClose(int nErrorCode)
{
	if (!m_nGssNetworkError)
		m_nGssNetworkError = -1;
	if (!nErrorCode)
		OnReceive(0);
	if (!m_nDecryptedReceiveBufferLen)
		TriggerEvent(FD_CLOSE, nErrorCode, TRUE);
}

int CALLBACK CAsyncGssSocketLayer::Callback(void *pData, int nParam1, int nParam2, int nParam3)
{
	CAsyncGssSocketLayer *pLayer = reinterpret_cast<CAsyncGssSocketLayer *>(pData);
	switch (nParam1)
	{
	case 0:
		pLayer->DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_INFO, nParam2);
		break;
	case 1:
		{
			char *buffer = (char *)nParam2;
			int len = strlen(buffer);
			if (nParam3)
			{
				pLayer->DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_COMMAND, nParam3);
				char *str = new char[strlen((char *)nParam2)+25];
				sprintf(str, "Encrypted command: %s", (char *)nParam2);
				pLayer->DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_INFO, (int)str);
				delete [] str;
			}
			else
				pLayer->DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_COMMAND, nParam2);
			pLayer->m_nAwaitingReply = nParam3 ? 2:1;
	
			if (!pLayer->m_pSendBuffer)
			{
				pLayer->m_pSendBuffer = new char[len + 4096];
				pLayer->m_nSendBufferSize = len + 4096;
			}
			else if (pLayer->m_nSendBufferSize < (pLayer->m_nSendBufferLen + len + 2))
			{
				char *tmp = pLayer->m_pSendBuffer;
				pLayer->m_pSendBuffer = new char[pLayer->m_nSendBufferLen + len + 4096];
				pLayer->m_nSendBufferSize = pLayer->m_nSendBufferLen + len + 4096;
				memcpy(pLayer->m_pSendBuffer, tmp, pLayer->m_nSendBufferLen);
				delete [] tmp;
			}

			memcpy(pLayer->m_pSendBuffer+pLayer->m_nSendBufferLen, (char *)nParam2, len);
			pLayer->m_pSendBuffer[pLayer->m_nSendBufferLen+len] = '\r';
			pLayer->m_pSendBuffer[pLayer->m_nSendBufferLen+len+1] = '\n';
			pLayer->m_nSendBufferLen += len + 2;
			pLayer->TriggerEvent(FD_WRITE, 0);
			break;
		}
	}
	return 0;
}

void CAsyncGssSocketLayer::ReceiveReply()
{
	if (m_nReceiveBufferSize < 4096)
	{
		delete [] m_pReceiveBuffer;
		m_pReceiveBuffer = new char[4096];
		m_nReceiveBufferSize = 4096;
	}

	char buffer;
	int numread = ReceiveNext(&buffer, 1);
	if (!numread)
	{
		m_nGssNetworkError = -1;
		
		//Trigger OnClose()
		TriggerEvent(FD_CLOSE, 0, TRUE);
		return;
	}
	else if ( numread == SOCKET_ERROR )
	{	
		if ( WSAGetLastError() != WSAEWOULDBLOCK )
		{
			m_nGssNetworkError = GetLastError();
			TriggerEvent(FD_CLOSE, 0, TRUE);
		}
		else if (m_nDecryptedReceiveBufferLen)
			TriggerEvent(FD_READ, 0, TRUE);
		return;
	}
	if (buffer == 0 || buffer=='\r' || buffer=='\n')
	{
		if (!m_nReceiveBufferLen)
			return;
		m_nAwaitingReply = 0;
		m_pReceiveBuffer[m_nReceiveBufferLen] = 0;	
		DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_REPLY, (int)m_pReceiveBuffer);
		int res = pFzGss_ProcessReply(m_pData, m_pReceiveBuffer);
		m_nReceiveBufferLen = 0;
		if (res == 1)
			DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_AUTHCOMPLETE, 0);
		else if (res==-1)
			return;
		else
		{
			m_gotAuth = 0;
			DoLayerCallback(LAYERCALLBACK_LAYERSPECIFIC, GSS_AUTHFAILED, 0);
		}
	}
	else if (m_nReceiveBufferLen < 4095)
		m_pReceiveBuffer[m_nReceiveBufferLen++] = buffer;
}

BOOL CAsyncGssSocketLayer::GetUserFromKrbTicket(char *buffer)
{
	if (!m_gotAuth || !m_bUseGSS || !m_pData || !pFzGss_GetUserFromKrbTicket)
		return FALSE;

	return pFzGss_GetUserFromKrbTicket(m_pData, buffer);
}
