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

#if !defined(AFX_LISTENSOCKET_H__9740B48D_6F4D_4C3B_B751_BC0BC65208C5__INCLUDED_)
#define AFX_LISTENSOCKET_H__9740B48D_6F4D_4C3B_B751_BC0BC65208C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListenSocket.h : Header-Datei
//


class CServerThread;
class CServer;

/////////////////////////////////////////////////////////////////////////////
// Befehlsziel CListenSocket 

class CListenSocket : public CAsyncSocketEx
{
// Attribute
public:

// Operationen
public:
	CListenSocket(CServer *pServer);
	virtual ~CListenSocket();

// Überschreibungen
public:
	BOOL m_bLocked;
	std::list<CServerThread*> *m_pThreadList;
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CListenSocket)
	public:
	virtual void OnAccept(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CListenSocket)
		// HINWEIS - Der Klassen-Assistent fügt hier Member-Funktionen ein und entfernt diese.
	//}}AFX_MSG

// Implementierung
protected:
	void SendStatus(CStdString status, int type);

	CServer *m_pServer;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_LISTENSOCKET_H__9740B48D_6F4D_4C3B_B751_BC0BC65208C5__INCLUDED_
