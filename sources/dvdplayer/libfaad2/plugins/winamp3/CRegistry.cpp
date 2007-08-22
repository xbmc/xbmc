/*
CRegistry class
Copyright (C) 2003 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
kreel@tiscali.it
*/

#include "CRegistry.h"
//#include <stdlib.h>

//************************************************************************************************

CRegistry::CRegistry()
{
	regKey=NULL;
	path=NULL;
}
//------------------------------------------------------------------------------------------------

CRegistry::~CRegistry()
{
	if(regKey)
		RegCloseKey(regKey);
	if(path)
		free(path);
}
//************************************************************************************************
/*
void CRegistry::ShowLastError(char *Caption)
{
LPVOID MsgBuf;
	if(FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &MsgBuf,
		0,
		NULL 
	))
		MessageBox(NULL, (LPCTSTR)MsgBuf, Caption, MB_OK|MB_ICONSTOP);
	if(MsgBuf)
		LocalFree(MsgBuf);
}*/
//************************************************************************************************

#define setPath(SubKey) \
	if(path) \
		free(path); \
	path=strdup(SubKey);

BOOL CRegistry::Open(HKEY hKey, char *SubKey)
{
	if(regKey)
		RegCloseKey(regKey);
	if(RegOpenKeyEx(hKey, SubKey, NULL , KEY_ALL_ACCESS , &regKey)==ERROR_SUCCESS)
	{
		setPath(SubKey);
		return TRUE;
	}
	else // can't open the key -> error
	{
		regKey=0;
		setPath("");
		return FALSE;
	}
}
//************************************************************************************************

BOOL CRegistry::OpenCreate(HKEY hKey, char *SubKey)
{
	if(regKey)
		RegCloseKey(regKey);
	if(RegOpenKeyEx(hKey, SubKey, NULL , KEY_ALL_ACCESS , &regKey)==ERROR_SUCCESS)
	{
		setPath(SubKey);
		return TRUE;
	}
	else // open failed -> create the key
	{
	DWORD disp;
		RegCreateKeyEx(hKey , SubKey, NULL , NULL, REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &regKey , &disp );
		if(disp==REG_CREATED_NEW_KEY) 
		{
			setPath(SubKey);
			return TRUE;
		}
		else // can't create the key -> error
		{
			regKey=0;
			setPath("");
			return FALSE;
		}
	}
}
//************************************************************************************************

void CRegistry::Close()
{
	if(regKey)
		RegCloseKey(regKey);
	regKey=NULL;
	if(path) 
		delete path; 
	path=NULL;
}
//************************************************************************************************
//************************************************************************************************
//************************************************************************************************

void CRegistry::DeleteVal(char *SubKey)
{
	RegDeleteValue(regKey,SubKey);
}
//************************************************************************************************

void CRegistry::DeleteKey(char *SubKey)
{
	RegDeleteKey(regKey,SubKey);
}

//************************************************************************************************
//************************************************************************************************
//************************************************************************************************

void CRegistry::SetBool(char *keyStr , BOOL val)
{
BOOL tempVal;
DWORD len;
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(BOOL));
}
//************************************************************************************************

void CRegistry::SetByte(char *keyStr , BYTE val)
{
DWORD	t=val;
DWORD	tempVal;
DWORD	len;
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&t , sizeof(DWORD));
}
//************************************************************************************************

void CRegistry::SetWord(char *keyStr , WORD val)
{
DWORD	t=val;
DWORD	tempVal;
DWORD	len;
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&t , sizeof(DWORD));
}
//************************************************************************************************

void CRegistry::SetDword(char *keyStr , DWORD val)
{
DWORD tempVal;
DWORD len;
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&val , sizeof(DWORD));
}
//************************************************************************************************

void CRegistry::SetFloat(char *keyStr , float val)
{
float tempVal;
DWORD len;
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS ||
		tempVal!=val)
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(float));
}
//************************************************************************************************

void CRegistry::SetStr(char *keyStr , char *valStr)
{
DWORD len;
DWORD slen=strlen(valStr)+1;

	if(!valStr || !*valStr)
		return;

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, NULL , &len )!=ERROR_SUCCESS ||
		len!=slen)
		RegSetValueEx(regKey , keyStr , NULL , REG_SZ , (BYTE *)valStr , slen);
	else
	{
	char *tempVal=new char[slen];
		if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)tempVal , &len )!=ERROR_SUCCESS ||
			strcmpi(tempVal,valStr))
			RegSetValueEx(regKey , keyStr , NULL , REG_SZ , (BYTE *)valStr , slen);
		delete tempVal;
	}
}
//************************************************************************************************

void CRegistry::SetValN(char *keyStr , BYTE *addr,  DWORD size)
{
DWORD len;
	if(!addr || !size)
		return;

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, NULL , &len )!=ERROR_SUCCESS ||
		len!=size)
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , addr , size);
	else
	{
	BYTE *tempVal=new BYTE[size];
		if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)tempVal , &len )!=ERROR_SUCCESS ||
			memcmp(tempVal,addr,len))
			RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , addr , size);
		delete tempVal;
	}
}



//************************************************************************************************
//************************************************************************************************
//************************************************************************************************



BOOL CRegistry::GetSetBool(char *keyStr, BOOL val)
{
BOOL tempVal;
DWORD len=sizeof(BOOL);

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(BOOL));
		return val;
	}
	return tempVal;
}
//************************************************************************************************

BYTE CRegistry::GetSetByte(char *keyStr, BYTE val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		tempVal=val;
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&tempVal , sizeof(DWORD));
		return val;
	}
	return (BYTE)tempVal;
}
//************************************************************************************************

WORD CRegistry::GetSetWord(char *keyStr, WORD val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		tempVal=val;
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&tempVal , sizeof(DWORD));
		return val;
	}
	return (WORD)tempVal;
}
//************************************************************************************************

DWORD CRegistry::GetSetDword(char *keyStr, DWORD val)
{
DWORD tempVal;
DWORD len=sizeof(DWORD);

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(regKey , keyStr , NULL , REG_DWORD , (BYTE *)&val , sizeof(DWORD));
		return val;
	}
	return (DWORD)tempVal;
}
//************************************************************************************************

float CRegistry::GetSetFloat(char *keyStr, float val)
{
float tempVal;
DWORD len=sizeof(float);

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)&tempVal , &len )!=ERROR_SUCCESS)
	{
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , (BYTE *)&val , sizeof(float));
		return val;
	}
	return tempVal;
}
//************************************************************************************************

int CRegistry::GetSetStr(char *keyStr, char *tempString, char *dest, int maxLen)
{
DWORD tempLen=maxLen;
	
	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *) dest , &tempLen )!=ERROR_SUCCESS)
	{
		if(!tempString)
		{
			*dest=0;
			return 0;
		}
		strcpy(dest,tempString);
		tempLen=strlen(tempString)+1;
		RegSetValueEx(regKey , keyStr , NULL , REG_SZ , (BYTE *)tempString , tempLen);
	}
	return tempLen;
}
//************************************************************************************************

int CRegistry::GetSetValN(char *keyStr, BYTE *tempAddr, BYTE *addr, DWORD size)
{
DWORD tempLen=size;

	if(RegQueryValueEx(regKey , keyStr , NULL , NULL, (BYTE *)addr , &tempLen )!=ERROR_SUCCESS)
	{
		if(!tempAddr)
		{
			*addr=0;
			return 0;
		}
		memcpy(addr,tempAddr,size);
		RegSetValueEx(regKey , keyStr , NULL , REG_BINARY , (BYTE *)addr , size);
	}
	return tempLen;
}
