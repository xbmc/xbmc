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

// AdminInterface.cpp: Implementierung der Klasse CAdminInterface.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AdminInterface.h"
#include "AdminSocket.h"
#include "Server.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CAdminInterface::CAdminInterface(CServer *pServer)
{
	ASSERT(pServer);
	m_pServer = pServer;
}

CAdminInterface::~CAdminInterface()
{
	for (std::list<CAdminSocket *>::iterator iter=m_AdminSocketList.begin(); iter!=m_AdminSocketList.end(); iter++)
		delete *iter;
}

BOOL CAdminInterface::Add(CAdminSocket *pAdminSocket)
{
	if (pAdminSocket->Init())
	{
		m_AdminSocketList.push_back(pAdminSocket);
		return TRUE;
	}
	return FALSE;
}

BOOL CAdminInterface::SendCommand(int nType, int nID, const void *pData, int nDataLength)
{
	ASSERT((!nDataLength && !pData) || (pData && nDataLength));

	std::list<CAdminSocket *> deleteList;
	std::list<CAdminSocket *>::iterator iter;
	for (iter=m_AdminSocketList.begin(); iter!=m_AdminSocketList.end(); iter++)
		if (!(*iter)->SendCommand(nType, nID, pData, nDataLength))
		{
			deleteList.push_back(*iter);
		}
	
	for (iter=deleteList.begin(); iter!=deleteList.end(); iter++)
		VERIFY(Remove(*iter));
	return TRUE;
}

BOOL CAdminInterface::Remove(CAdminSocket *pAdminSocket)
{
	for (std::list<CAdminSocket *>::iterator iter=m_AdminSocketList.begin(); iter!=m_AdminSocketList.end(); iter++)
		if (*iter == pAdminSocket)
		{
			delete *iter;
			m_AdminSocketList.erase(iter);
			return TRUE;
		}
	return FALSE;
}

BOOL CAdminInterface::ProcessCommand(CAdminSocket *pAdminSocket, int nID, void *pData, int nDataLength)
{
	m_pServer->ProcessCommand(pAdminSocket, nID, reinterpret_cast<unsigned char *>(pData), nDataLength);
	return TRUE;
}

void CAdminInterface::CheckForTimeout()
{
	std::list<CAdminSocket *> deleteList;
	std::list<CAdminSocket *>::iterator iter;
	for (iter=m_AdminSocketList.begin(); iter!=m_AdminSocketList.end(); iter++)
		if ((*iter)->CheckForTimeout())
			deleteList.push_back(*iter);
		
	for (iter=deleteList.begin(); iter!=deleteList.end(); iter++)
		VERIFY(Remove(*iter));
}
