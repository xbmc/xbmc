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

// AdminListenSocket.h: Schnittstelle für die Klasse CAdminListenSocket.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADMINLISTENSOCKET_H__F48CDEB4_67A4_47B6_B46F_5843DFC4A251__INCLUDED_)
#define AFX_ADMINLISTENSOCKET_H__F48CDEB4_67A4_47B6_B46F_5843DFC4A251__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AsyncSocketEx.h"

class CAdminInterface;
class CAdminListenSocket : public CAsyncSocketEx  
{
public:
	CAdminListenSocket(CAdminInterface *pAdminInterface);
	virtual ~CAdminListenSocket();

protected:
	virtual void OnAccept(int nErrorCode);

	CAdminInterface *m_pAdminInterface;
};

#endif // !defined(AFX_ADMINLISTENSOCKET_H__F48CDEB4_67A4_47B6_B46F_5843DFC4A251__INCLUDED_)
