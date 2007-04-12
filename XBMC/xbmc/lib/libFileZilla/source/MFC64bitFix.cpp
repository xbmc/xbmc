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

#include "stdafx.h"

#include "MFC64bitFix.h"

/*__int64 GetLength64(CFile &file)
{
	DWORD low;
	DWORD high;
	low=GetFileSize((void *)file.m_hFile, &high);
	_int64 size=((_int64)high<<32)+low;
	return size;
}*/

BOOL GetLength64(LPCTSTR filename, _int64 &size)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(filename, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	VERIFY(FindClose(hFind));

	size=((_int64)findFileData.nFileSizeHigh<<32)+findFileData.nFileSizeLow;
	
	return TRUE;	
}

BOOL PASCAL GetStatus64(LPCTSTR lpszFileName, CFileStatus64& rStatus)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((LPTSTR)lpszFileName, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;
	VERIFY(FindClose(hFind));

	// strip attribute of NORMAL bit, our API doesn't have a "normal" bit.
	rStatus.m_attribute = (BYTE)
		(findFileData.dwFileAttributes & ~FILE_ATTRIBUTE_NORMAL);

	rStatus.m_size = ((_int64)findFileData.nFileSizeHigh<<32)+findFileData.nFileSizeLow;

	// convert times as appropriate
	rStatus.m_ctime = findFileData.ftCreationTime;
	rStatus.m_atime = findFileData.ftLastAccessTime;
	rStatus.m_mtime = findFileData.ftLastWriteTime;

	if (rStatus.m_ctime.dwHighDateTime == rStatus.m_ctime.dwLowDateTime == 0)
		rStatus.m_ctime = rStatus.m_mtime;

	if (rStatus.m_atime.dwHighDateTime == rStatus.m_atime.dwLowDateTime == 0)
		rStatus.m_atime = rStatus.m_mtime;

	return TRUE;
}

_int64 GetPosition64(HANDLE hFile)
{
	if (!hFile || hFile==INVALID_HANDLE_VALUE)
		return -1;
	LONG low=0;
	LONG high=0;
	low=SetFilePointer(hFile, low, &high, FILE_CURRENT);
	if (low==0xFFFFFFFF && GetLastError!=NO_ERROR)
		return -1;
	return ((_int64)high<<32)+low;
}