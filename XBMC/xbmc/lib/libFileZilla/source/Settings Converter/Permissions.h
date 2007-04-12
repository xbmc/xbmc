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

// Permissions.h: Schnittstelle für die Klasse CPermissions.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PERMISSIONS_H__33DEA50E_AA34_4190_9ACD_355BF3D72FE0__INCLUDED_)
#define AFX_PERMISSIONS_H__33DEA50E_AA34_4190_9ACD_355BF3D72FE0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "misc\stdstring.h"
#include <vector>

typedef struct
{
	CStdString dir;
	BOOL bFileRead,bFileWrite,bFileDelete,bFileAppend;
	BOOL bDirCreate,bDirDelete,bDirList,bDirSubdirs,bIsHome;
} t_directory;

class t_user
{
public:
	CStdString user;
	CStdString password;
	std::vector<t_directory> permissions;
	BOOL bLnk, bRelative, bBypassUserLimit;
	int nUserLimit, nIpLimit;
	t_user& t_user::operator=(const t_user &a);
};

class CMarkupSTL;
class CPermissions  
{
public:
	CPermissions(BOOL bCU = TRUE);
	virtual ~CPermissions();

	BOOL Convert(CMarkupSTL *pXML);

protected:
	CStdString GetKey(const CStdString &subkey, const CStdString &keyname);
	void ReadPermissions(t_user &user);
	void SetKey(CMarkupSTL *pXML, LPCTSTR name, LPCTSTR value);
	void SavePermissions(CMarkupSTL *pXML, const t_user &user);
		
	typedef std::vector<t_user> t_UsersList; 
	t_UsersList m_UsersList;

	BOOL m_bCU;
};

#endif // !defined(AFX_PERMISSIONS_H__33DEA50E_AA34_4190_9ACD_355BF3D72FE0__INCLUDED_)
