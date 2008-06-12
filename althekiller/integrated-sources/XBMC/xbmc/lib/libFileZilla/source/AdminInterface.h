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

// AdminInterface.h: Schnittstelle für die Klasse CAdminInterface.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADMININTERFACE_H__1C8681AA_4200_417C_B638_D5E39AD896E1__INCLUDED_)
#define AFX_ADMININTERFACE_H__1C8681AA_4200_417C_B638_D5E39AD896E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CAdminSocket;
class CServer;
class CAdminInterface  
{
public:
	void CheckForTimeout();
	BOOL ProcessCommand(CAdminSocket *pAdminSocket, int nID, void *pData, int nDataLength);
	BOOL Add(CAdminSocket *pAdminSocket);
	CAdminInterface(CServer *pServer);
	virtual ~CAdminInterface();
	BOOL SendCommand(int nType, int nID, const void *pData, int nDataLength);
	BOOL Remove(CAdminSocket *pAdminSocket);
	
protected:
	CServer *m_pServer;

	std::list<CAdminSocket *> m_AdminSocketList;
};

#endif // !defined(AFX_ADMININTERFACE_H__1C8681AA_4200_417C_B638_D5E39AD896E1__INCLUDED_)
