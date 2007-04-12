// AdminListenSocket.cpp: Implementierung der Klasse CAdminListenSocket.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AdminListenSocket.h"
#include "AdminSocket.h"
#include "AdminInterface.h"
#include "Options.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CAdminListenSocket::CAdminListenSocket(CAdminInterface *pAdminInterface)
{
	ASSERT(pAdminInterface);
	m_pAdminInterface = pAdminInterface;
}

CAdminListenSocket::~CAdminListenSocket()
{
}

void CAdminListenSocket::OnAccept(int nErrorCode)
{
	CAdminSocket *pSocket = new CAdminSocket(m_pAdminInterface);

	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	
	if (Accept(*pSocket, (SOCKADDR*)&sockAddr, &nSockAddrLen))
	{
		//Validate IP address
		CStdString ip = inet_ntoa(sockAddr.sin_addr);
		COptions options;
		CStdString validIPs = options.GetOption(OPTION_ADMINIPADDRESSES);

		if (validIPs != "")
			validIPs += " ";
		int pos = validIPs.Find(" ");
		BOOL bMatchFound = (ip != "127.0.0.1" ? FALSE : TRUE);
		while (pos!=-1 && !bMatchFound)
		{
			CStdString curIp = validIPs.Left(pos);
			LPCTSTR c = curIp;
			LPCTSTR p = ip;

			//Look if remote and local ip match
			while (*c && *p)
			{
				if (*c == '*')
				{
					if (*p == '.')
						break;

					//Advance to next dot
					while (*p && *p != '.')
						p++;
					c++;
					continue;
				}
				else if (*c != '?') //If it's '?', we don't have to compare
					if (*c != *p)
						break;
				p++;
				c++;
			}
			if (!*c && !*p)
				bMatchFound = TRUE;

			validIPs = validIPs.Mid(pos+1);
			pos = validIPs.Find(" ");
		}

		if (!bMatchFound)
		{
			delete pSocket;
			return;
		}

		pSocket->AsyncSelect();
		if (!m_pAdminInterface->Add(pSocket))
			delete pSocket;
	}
}