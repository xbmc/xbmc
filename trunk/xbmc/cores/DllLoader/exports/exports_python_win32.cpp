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
 
#include <io.h>
#include <direct.h>
#include <sys/utime.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../DllLoader.h"
#include "../../../FileSystem/SpecialProtocol.h"
#include "utils/log.h"

#include "../dll_tracker_file.h"
#include "emu_msvcrt.h"

extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);

// file io functions
#define CORRECT_SEP_STR(str) \
  if (strstr(str, "://") == NULL) \
  { \
    int iSize_##str = strlen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) \
      if (str[pos] == '/') str[pos] = '\\'; \
  } \
  else \
  { \
    int iSize_##str = strlen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) \
      if (str[pos] == '\\') str[pos] = '/'; \
  }

#define CORRECT_SEP_WSTR(str) \
  if (wcsstr(str, L"://") == NULL) \
  { \
    int iSize_##str = wcslen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) \
      if (str[pos] == '/') str[pos] = '\\'; \
  } \
  else \
  { \
    int iSize_##str = wcslen(str); \
    for (int pos = 0; pos < iSize_##str; pos++) \
      if (str[pos] == '\\') str[pos] = '/'; \
  }

extern "C"
{
  static char xbp_cw_dir[MAX_PATH] = "";

  char* xbp_getcwd(char *buf, int size)
  {
    if (buf == NULL) buf = (char *)malloc(size);
    strcpy(buf, xbp_cw_dir);
    return buf;
  }

  int xbp_chdir(const char *dirname)
  {
    if (strlen(dirname) > MAX_PATH) return -1;

    strcpy(xbp_cw_dir, dirname);
    CORRECT_SEP_STR(xbp_cw_dir);

    return 0;
  }

  char* xbp__tempnam(const char *dir, const char *prefix)
  {
    char* p = strdup(dir);
    CORRECT_SEP_STR(p);
    char* res = _tempnam(p, prefix);
    free(p);
    return strdup(res);
  }

  int xbp_unlink(const char *filename)
  {
    char* p = strdup(filename);
    CORRECT_SEP_STR(p);
    int res = unlink(_P(p).c_str());
    free(p);
    return res;
  }

  int xbp_access(const char *path, int mode)
  {
    char* p = strdup(path);
    CORRECT_SEP_STR(p);
    int res = access(_P(p).c_str(), mode);
    free(p);
    return res;
  }

  int xbp_chmod(const char *filename, int pmode)
  {
    char* p = strdup(filename);
    CORRECT_SEP_STR(p);
    int res = chmod(_P(p).c_str(), pmode);
    free(p);
    return res;
  }

  int xbp_rmdir(const char *dirname)
  {
    char* p = strdup(dirname);
    CORRECT_SEP_STR(p);
    int res = rmdir(_P(p).c_str());
    free(p);
    return res;
  }

  int xbp_utime(const char *filename, struct utimbuf *times)
  {
    char* p = strdup(filename);
    CORRECT_SEP_STR(p);
    int res = utime(_P(p).c_str(), times);
    free(p);
    return res;
  }

  int xbp_rename(const char *oldname, const char *newname)
  {
    char* o = strdup(oldname);
    char* n = strdup(newname);
    CORRECT_SEP_STR(o);
    CORRECT_SEP_STR(n);
    int res = rename(_P(o).c_str(), _P(n).c_str());
    free(o);
    free(n);
    return res;
  }

  int xbp_mkdir(const char *dirname)
  {
    char* p = strdup(dirname);
    CORRECT_SEP_STR(p);
    int res = mkdir(_P(p).c_str());
    free(p);
    return res;
  }

  int xbp_open(const char *filename, int oflag, int pmode)
  {
    char* p = strdup(filename);
    CORRECT_SEP_STR(p);
    int res = open(_P(p).c_str(), oflag, pmode);
    free(p);
    return res;
  }

  FILE* xbp_fopen(const char *filename, const char *mode)
  {
    //convert '/' to '\\'
    char cName[1024];
    char* p;
    
    strcpy(cName, filename);
    CORRECT_SEP_STR(cName);
    
    //for each "\\..\\" remove the directory before it
    while(p = strstr(cName, "\\..\\"))
    {
      char* file = p + 3;
      *p = '\0';
      *strrchr(cName, '\\') = '\0';
      strcat(cName, file);
    }

    // don't use emulated files, they do not work in python yet
    return fopen_utf8(_P(cName).c_str(), mode);
  }

  #if 0 // TODO: Is this needed?
  FILE* xbp_fopen64(const char *filename, const char *mode)
  {
    CStdString strName = filename;
    printf("....%s\n", strName.c_str());
    // don't use emulated files, they do not work in python yet
    return fopen64(_P(strName).c_str(), mode);
  }
  #endif

  int xbp_fclose(FILE* stream)
  {
    return dll_fclose(stream);
  }

  int xbp_close(int fd)
  {
    return dll_close(fd);
  }

  FILE* xbp__wfopen(const wchar_t *filename, const wchar_t *mode)
  {
    CLog::Log(LOGERROR, "xbp__wfopen = untested");
    wchar_t* p = wcsdup(filename);
    CORRECT_SEP_WSTR(p);
    FILE* res = _wfopen(p, mode);
    free(p);
    return res;
  }

  BOOL xbp_FindClose(HANDLE hFindFile)
  {
    return FindClose(hFindFile);
  }

  HANDLE xbp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
  {
    char* p = strdup(lpFileName);
    CORRECT_SEP_STR(p);
    
    // change default \\*.* into \\* which the xbox is using
    char* e = strrchr(p, '.');
    if (e != NULL && strlen(e) > 1 && e[1] == '*')
    {
      e[0] = '\0';
    }
    
    HANDLE res = FindFirstFile(_P(p).c_str(), lpFindFileData);
    free(p);
    return res;
  }

  BOOL xbp_FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
  {
    return FindNextFile(hFindFile, lpFindFileData);
  }

  long xbp__get_osfhandle(int fd)
  {
    return _get_osfhandle(fd);
  }

  int xbp_dup(int fd)
  {
    return dup(fd);
  }

  int xbp_dup2(int fd1, int fd2)
  {
    return dup2(fd1, fd2);
  }

} // extern "C"
