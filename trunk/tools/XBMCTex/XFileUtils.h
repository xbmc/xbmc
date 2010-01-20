#ifndef __X_FILE_UTILS_
#define __X_FILE_UTILS_

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PlatformDefs.h"
#include "XHandlePublic.h"

#ifdef _LINUX
#define XBMC_FILE_SEP '/'
#else
#define XBMC_FILE_SEP '\\'
#endif

HANDLE FindFirstFile(LPCSTR,LPWIN32_FIND_DATA);

BOOL   FindNextFile(HANDLE,LPWIN32_FIND_DATA);
BOOL   FindClose(HANDLE hFindFile);

int    _stat64(const char *path, struct __stat64 *buffer);
int    _stati64(const char *path,struct _stati64 *buffer);

DWORD  GetFileAttributes(LPCTSTR lpFileName);

#define ERROR_ALREADY_EXISTS EEXIST

#endif
