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

#include "system.h"
#include "PlatformInclude.h"
#include "XFileUtils.h"
#include "XTimeUtils.h"
#include "Util.h"
#include "XHandle.h"

#ifdef _LINUX

#include <sys/types.h>
#include <sys/stat.h>
#ifndef __APPLE__
#include <sys/vfs.h>
#else
#include <sys/param.h>
#endif
#include <regex.h>
#include <dirent.h>
#include <errno.h>


HANDLE FindFirstFile(LPCSTR szPath,LPWIN32_FIND_DATA lpFindData) {
  if (lpFindData == NULL || szPath == NULL)
    return NULL;

  CStdString strPath(szPath);

  if (strPath.empty())
    return INVALID_HANDLE_VALUE;

   strPath.Replace("\\","/");

  // if the file name is a directory then we add a * to look for all files in this directory
  DIR *testDir = opendir(szPath);
  if (testDir) {
    strPath += "/*";
    closedir(testDir);
  }

  int nFilePos = strPath.ReverseFind(XBMC_FILE_SEP);

  CStdString strDir = ".";
  CStdString strFiles = strPath;

  if (nFilePos > 0) {
    strDir = strPath.substr(0,nFilePos);
    strFiles = strPath.substr(nFilePos + 1);
  }

        if (strFiles == "*.*")
           strFiles = "*";

  strFiles = CStdString("^") + strFiles + "$";
  strFiles.Replace(".","\\.");
  strFiles.Replace("*",".*");
  strFiles.Replace("?",".");

  strFiles.MakeLower();

  int status;
  regex_t re;
  if (regcomp(&re, strFiles, REG_EXTENDED|REG_NOSUB) != 0) {
    return(INVALID_HANDLE_VALUE);
  }

  struct dirent **namelist = NULL;
  int n = scandir(strDir, &namelist, 0, alphasort);

  CXHandle *pHandle = new CXHandle(CXHandle::HND_FIND_FILE);
    pHandle->m_FindFileDir = strDir;

  while (n-- > 0) {
    CStdString strComp(namelist[n]->d_name);
    strComp.MakeLower();

    status = regexec(&re, strComp.c_str(), (size_t) 0, NULL, 0);
    if (status == 0) {
      pHandle->m_FindFileResults.push_back(namelist[n]->d_name);
    }
    free(namelist[n]);
  }

  if (namelist)
    free(namelist);

  regfree(&re);

  if (pHandle->m_FindFileResults.size() == 0) {
    delete pHandle;
    return INVALID_HANDLE_VALUE;
  }

  FindNextFile(pHandle, lpFindData);

  return pHandle;
}

BOOL   FindNextFile(HANDLE hHandle, LPWIN32_FIND_DATA lpFindData) {
  if (lpFindData == NULL || hHandle == NULL || hHandle->GetType() != CXHandle::HND_FIND_FILE)
    return FALSE;

  if ((unsigned int) hHandle->m_nFindFileIterator >= hHandle->m_FindFileResults.size())
    return FALSE;

  CStdString strFileName = hHandle->m_FindFileResults[hHandle->m_nFindFileIterator++];
        CStdString strFileNameTest = hHandle->m_FindFileDir + '/' + strFileName;

  struct stat64 fileStat;
  if (stat64(strFileNameTest, &fileStat) != 0)
    return FALSE;

  bool bIsDir = false;
  DIR *testDir = opendir(strFileNameTest);
  if (testDir) {
    bIsDir = true;
    closedir(testDir);
  }

  memset(lpFindData,0,sizeof(WIN32_FIND_DATA));

  lpFindData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
  strcpy(lpFindData->cFileName, strFileName.c_str());

  if (bIsDir)
    lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

  if (strFileName[0] == '.')
    lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;

  if (access(strFileName, R_OK) == 0 && access(strFileName, W_OK) != 0)
    lpFindData->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

  TimeTToFileTime(fileStat.st_ctime, &lpFindData->ftCreationTime);
  TimeTToFileTime(fileStat.st_atime, &lpFindData->ftLastAccessTime);
  TimeTToFileTime(fileStat.st_mtime, &lpFindData->ftLastWriteTime);

  lpFindData->nFileSizeHigh = (DWORD)(fileStat.st_size >> 32);
  lpFindData->nFileSizeLow =  (DWORD)fileStat.st_size;

  return TRUE;
}

BOOL   FindClose(HANDLE hFindFile) {
  return CloseHandle(hFindFile);
}

int _stat64(   const char *path,   struct __stat64 *buffer ) {
  if (buffer == NULL || path == NULL)
    return -1;

  return stat64(path, buffer);
}

DWORD  GetFileAttributes(LPCTSTR lpFileName)
{
  if (lpFileName == NULL) {
    return 0;
  }

  DWORD dwAttr = FILE_ATTRIBUTE_NORMAL;
  DIR *tmpDir = opendir(lpFileName);
  if (tmpDir) {
    dwAttr |= FILE_ATTRIBUTE_DIRECTORY;
    closedir(tmpDir);
  }

  if (lpFileName[0] == '.')
    dwAttr |= FILE_ATTRIBUTE_HIDDEN;

  if (access(lpFileName, R_OK) == 0 && access(lpFileName, W_OK) != 0)
    dwAttr |= FILE_ATTRIBUTE_READONLY;

  return dwAttr;
}

#endif

