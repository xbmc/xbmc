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

#include "Accounts.h"

#define FOP_READ 1
#define FOP_WRITE 2
#define FOP_DELETE 4
#define FOP_APPEND 8
#define FOP_CREATENEW 0x10
#define DOP_DELETE 16
#define DOP_CREATE 32

#define PERMISSION_DENIED 0x01
#define PERMISSION_NOTFOUND 0x02
#define PERMISSION_DIRNOTFILE (0x04 | PERMISSION_DOESALREADYEXIST)
#define PERMISSION_FILENOTDIR (0x08 | PERMISSION_DOESALREADYEXIST)
#define PERMISSION_DOESALREADYEXIST 0x10

class CMarkupSTL;
class CPermissionsHelperWindow;
class COptions;

class CUser : public t_user
{
public:
	CStdString homedir;
};

struct t_dirlisting
{
	char buffer[8192];
	int len;

	t_dirlisting *pNext;
};

class CPermissions  
{
public:
	CPermissions();
	virtual ~CPermissions();

	int ChangeCurrentDir(LPCTSTR user, CStdString& currentdir, CStdString &dir);
	int GetDirectoryListing(LPCTSTR user, CStdString dir, t_dirlisting *&pResult);
	int GetDirName(LPCTSTR user, CStdString dirname, CStdString currentdir, int op, CStdString &physical, CStdString &logical);
	int GetFileName(LPCTSTR user, CStdString filename, CStdString currentdir, int op, CStdString &result);
	CStdString GetHomeDir(LPCTSTR username, BOOL bRealPath = FALSE) const;
	CStdString GetHomeDir(const CUser &user, BOOL bRealPath = FALSE) const;
	CStdString GetHomeDir(unsigned int index, BOOL bRealPath = FALSE) const;
	int GetShortDirectoryListing(LPCTSTR user, CStdString currentDir, CStdString dirToDisplay, t_dirlisting *&pResult);
	BOOL GetUser(CStdString user, CUser &userdata) const;
	BOOL Lookup(LPCTSTR user, LPCTSTR pass, CUser &userdata, BOOL noPasswordCheck = FALSE);
	BOOL GetAsCommand(char **pBuffer, DWORD *nBufferLength);
	BOOL ParseUsersCommand(unsigned char *pData, DWORD dwDataLength);
	void AutoCreateDirs(const char *username);
	void ReloadConfig();
	
protected:
	BOOL Init();
	int GetRealDirectory(CStdString directory, int user, t_directory &ret, BOOL &truematch);
	CStdString GetShortcutTarget(LPCTSTR filename);
	void UpdateInstances();

	void ReadPermissions(CMarkupSTL *pXML, t_group &user, BOOL &bGotHome);
	void SavePermissions(CMarkupSTL *pXML, const t_group &user);

	void ReadSpeedLimits(CMarkupSTL *pXML, t_group &group);
	void SaveSpeedLimits(CMarkupSTL *pXML, const t_group &group);

	void SetKey(CMarkupSTL *pXML, LPCTSTR name, LPCTSTR value);
	
		
	static CCriticalSectionWrapper m_sync;
	
	typedef std::vector<CUser> t_UsersList; 
	typedef std::vector<t_group> t_GroupsList; 
	static t_UsersList m_sUsersList;
	static t_GroupsList m_sGroupsList;
	t_UsersList m_UsersList;
	t_GroupsList m_GroupsList;
	
	static std::list<CPermissions *> m_sInstanceList;
	CPermissionsHelperWindow *m_pPermissionsHelperWindow;
	friend CPermissionsHelperWindow;
};

#endif // !defined(AFX_PERMISSIONS_H__33DEA50E_AA34_4190_9ACD_355BF3D72FE0__INCLUDED_)
