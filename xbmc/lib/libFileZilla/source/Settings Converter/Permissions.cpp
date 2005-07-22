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

// Permissions.cpp: Implementierung der Klasse CPermissions.
//
//////////////////////////////////////////////////////////////////////

#include "Permissions.h"
#include "misc\MarkupSTL.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPermissions

t_user& t_user::operator=(const t_user &a)
{
	user=a.user;
	password=a.password;
	bLnk=a.bLnk;
	bRelative=a.bRelative;
	bBypassUserLimit=a.bBypassUserLimit;
	nUserLimit=a.nUserLimit;
	nIpLimit=a.nIpLimit;
	for (std::vector<t_directory>::const_iterator iter=a.permissions.begin(); iter!=a.permissions.end(); iter++)
		permissions.push_back(*iter);
	return *this;
}

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CPermissions::CPermissions(BOOL bCU /*=TRUE*/)
{
	m_bCU = bCU;
	HKEY key;
	if (RegOpenKey(m_bCU?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE, "Software\\FileZilla Server\\Users\\", &key)==ERROR_SUCCESS)
	{
		char buffer[1000];
		int index=0;
		while (RegEnumKey(key,index,buffer,1000)==ERROR_SUCCESS)
		{
			t_user user;
			user.user=buffer;
			user.password=GetKey(user.user,"Pass");
			user.bLnk=GetKey(user.user,"Resolve Shortcuts")=="1";
			user.bRelative=GetKey(user.user,"Relative")=="1";
			user.bBypassUserLimit=GetKey(user.user,"Bypass server userlimit")=="1";
			user.nUserLimit=atoi(GetKey(user.user,"User Limit"));
			if (user.nUserLimit<0 || user.nUserLimit>999999999)
				user.nUserLimit=0;
			user.nIpLimit=atoi(GetKey(user.user,"IP Limit"));
			if (user.nIpLimit<0 || user.nIpLimit>999999999)
				user.nIpLimit=0;
			ReadPermissions(user);
			m_UsersList.push_back(user);
			index++;
		}
		RegCloseKey(key);
	}
}

CPermissions::~CPermissions()
{
}

CStdString CPermissions::GetKey(const CStdString &subkey, const CStdString &keyname)
{
	HKEY key;
	if (RegOpenKey(m_bCU?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE, "Software\\FileZilla Server\\Users\\"+subkey, &key)==ERROR_SUCCESS)
	{
		unsigned char *buffer=new unsigned char[100];
		memset(buffer,0,100);
	
		unsigned long tmp=100;
		if (RegQueryValueEx(key,keyname,NULL,NULL,buffer,&tmp)!=ERROR_SUCCESS) 
		{
			RegCloseKey(key);
			delete [] buffer;
			return "";
		}
		else 
		{
			RegCloseKey(key);
			CStdString res=reinterpret_cast<char *>(buffer);
			delete [] buffer;
			return res;
		}
	}
	return "";
}

void CPermissions::ReadPermissions(t_user &user)
{
	HKEY key;
	if (RegOpenKey(m_bCU?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE, "Software\\FileZilla Server\\Users\\"+user.user+"\\", &key)==ERROR_SUCCESS)
	{
		char buffer[1000];
		int index=0;
		while (RegEnumKey(key,index,buffer,1000)==ERROR_SUCCESS)
		{
			CStdString buffer2=buffer;
			t_directory dir;
			dir.dir=GetKey(user.user+"\\"+buffer2,"");
			dir.bFileRead=GetKey(user.user+"\\"+buffer2,"FileRead")=="1";
			dir.bFileWrite=GetKey(user.user+"\\"+buffer2,"FileWrite")=="1";
			dir.bFileDelete=GetKey(user.user+"\\"+buffer2,"FileDelete")=="1";
			dir.bFileAppend=GetKey(user.user+"\\"+buffer2,"FileAppend")=="1";
			dir.bDirCreate=GetKey(user.user+"\\"+buffer2,"DirCreate")=="1";
			dir.bDirDelete=GetKey(user.user+"\\"+buffer2,"DirDelete")=="1";
			dir.bDirList=GetKey(user.user+"\\"+buffer2,"DirList")=="1";
			dir.bDirSubdirs=GetKey(user.user+"\\"+buffer2,"DirSubdirs")=="1";
			dir.bIsHome=GetKey(user.user+"\\"+buffer2,"IsHome")=="1";
			user.permissions.push_back(dir);
			index++;
		}
		RegCloseKey(key);
	}
}

void CPermissions::SetKey(CMarkupSTL *pXML, LPCTSTR name, LPCTSTR value)
{
	ASSERT(pXML);
	pXML->AddChildElem(_T("Option"), value);
	pXML->AddChildAttrib(_T("Name"), name);
}

void CPermissions::SavePermissions(CMarkupSTL *pXML, const t_user &user)
{
	pXML->AddChildElem(_T("Permissions"));
	pXML->IntoElem();
	for (int i=0;i<user.permissions.size();i++)
	{
		pXML->AddChildElem(_T("Permission"));
		pXML->AddChildAttrib(_T("Dir"), user.permissions[i].dir);
		pXML->IntoElem();
		SetKey(pXML, "FileRead", user.permissions[i].bFileRead ? "1":"0");
		SetKey(pXML, "FileWrite", user.permissions[i].bFileWrite ? "1":"0");
		SetKey(pXML, "FileDelete", user.permissions[i].bFileDelete ?"1":"0");
		SetKey(pXML, "FileAppend", user.permissions[i].bFileAppend ? "1":"0");
		SetKey(pXML, "DirCreate", user.permissions[i].bDirCreate ? "1":"0");
		SetKey(pXML, "DirDelete", user.permissions[i].bDirDelete ? "1":"0");
		SetKey(pXML, "DirList", user.permissions[i].bDirList ? "1":"0");
		SetKey(pXML, "DirSubdirs", user.permissions[i].bDirSubdirs ? "1":"0");	
		SetKey(pXML, "IsHome", user.permissions[i].bIsHome ? "1":"0");	
		pXML->OutOfElem();
	}
	pXML->OutOfElem();
}

BOOL CPermissions::Convert(CMarkupSTL *pXML)
{
	if (!m_UsersList.size())
		return FALSE;

	if (!pXML->FindChildElem(_T("Users")))
		pXML->AddChildElem(_T("Users"));
	pXML->IntoElem();
		
	//Save the changed user details
	for (t_UsersList::const_iterator iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
	{
		pXML->ResetChildPos();
		CStdString newname = iter->user;
		newname.MakeLower();
		while (pXML->FindChildElem("User"))
		{
			CStdString name = pXML->GetChildAttrib(_T("Name"));
			name.MakeLower();
			if (name == newname)
				pXML->RemoveChildElem();
		}
		pXML->AddChildElem(_T("User"));
		pXML->AddChildAttrib(_T("Name"), iter->user);
		pXML->IntoElem();
		SetKey(pXML, "Pass", iter->password);
		SetKey(pXML, "Resolve Shortcuts", iter->bLnk?"1":"0");
		SetKey(pXML, "Relative", iter->bRelative?"1":"0");
		SetKey(pXML, "Bypass server userlimit", iter->bBypassUserLimit?"1":"0");
		CStdString str;
		str.Format(_T("%d"), iter->nUserLimit);
		SetKey(pXML, "User Limit", str);
		str.Format(_T("%d"), iter->nIpLimit);
		SetKey(pXML, "IP Limit", str);

		SavePermissions(pXML, *iter);
		pXML->OutOfElem();
	}
	return TRUE;
}
