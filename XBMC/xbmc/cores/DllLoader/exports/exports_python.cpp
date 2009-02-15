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
 
#include "stdafx.h"
#ifndef _LINUX
#include <io.h>
#include <direct.h>
#include <sys/utime.h>
#else
#include <sys/types.h>
#include <utime.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <stdio.h>

#include "../DllLoader.h"
#include "../../../FileSystem/SpecialProtocol.h"

#ifdef _WIN32PC
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

// file io functions
#ifndef _LINUX
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
#else

#define CORRECT_SEP_STR(str)
#define CORRECT_SEP_WSTR(str)

#endif

#include "../dll_tracker_file.h"
#include "emu_msvcrt.h"

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
#ifndef _LINUX
  char* p = strdup(dir);
  CORRECT_SEP_STR(p);
  char* res = _tempnam(p, prefix);
  free(p);
  return strdup(res);
#else
  CStdString result = dir;
  result += "/";
  result += prefix;
  result += tmpnam(NULL);
  return strdup(result.c_str());
#endif
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
#ifndef _LINUX
  int res = mkdir(_P(p).c_str());
#else
  int res = mkdir(_P(p).c_str(), 0755);
#endif
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

#ifdef _LINUX
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
#ifndef _LINUX
  wchar_t* p = wcsdup(filename);
  CORRECT_SEP_WSTR(p);
  FILE* res = _wfopen(_P(p).c_str(), mode);
  free(p);
  return res;
#else
  return NULL;
#endif
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
#ifndef _LINUX
  return _get_osfhandle(fd);
#else
  return fd;
#endif
}

int xbp_dup(int fd)
{
  return dup(fd);
}

int xbp_dup2(int fd1, int fd2)
{
  return dup2(fd1, fd2);
}

#ifdef _LINUX
DIR *xbp_opendir(const char *name)
{
  return opendir(_P(name).c_str());
}
#endif

} // extern "C"

Export export_xbp[] =
{
  { "xbp_fgetc",                  -1, (void*) dll_fgetc,                     NULL },
  { "xbp_clearerr",               -1, (void*) dll_clearerr,                  NULL },
  //{ "xbp__stati64",               -1, (void*) xbp__stati64,                  NULL },
  { "xbp__fstati64",              -1, (void*) dll_fstati64,                  NULL },
  { "xbp__tempnam",               -1, (void*) xbp__tempnam,                  NULL },
  { "xbp_tmpfile",                -1, (void*) tmpfile,                       NULL },
  { "xbp_tmpnam",                 -1, (void*) tmpnam,                        NULL },
  { "xbp__wfopen",                -1, (void*) xbp__wfopen,                   NULL },
  { "xbp__get_osfhandle",         -1, (void*) xbp__get_osfhandle,            NULL },
  //{ "xbp_vfprintf",               -1, (void*) xbp_vfprintf,                  NULL },
  { "xbp_unlink",                 -1, (void*) xbp_unlink,                    NULL },
  { "xbp_access",                 -1, (void*) xbp_access,                    NULL },
  { "xbp_chdir",                  -1, (void*) xbp_chdir,                     NULL },
  { "xbp_chmod",                  -1, (void*) xbp_chmod,                     NULL },
  { "xbp_rmdir",                  -1, (void*) xbp_rmdir,                     NULL },
  { "xbp_umask",                  -1, (void*) umask,                         NULL },
  { "xbp_utime",                  -1, (void*) xbp_utime,                     NULL },
  { "xbp_dup",                    -1, (void*) xbp_dup,                       NULL },
  { "xbp_dup2",                   -1, (void*) xbp_dup2,                      NULL },
  { "xbp_rename",                 -1, (void*) xbp_rename,                    NULL },
  { "xbp__commit",                -1, (void*) dll__commit,                       NULL },
  { "xbp__setmode",               -1, (void*) dll_setmode,                   NULL },
  { "xbp_fopen",                  -1, (void*) xbp_fopen,                     NULL },
  { "xbp_ungetc",                 -1, (void*) dll_ungetc,                    NULL },
  { "xbp_fflush",                 -1, (void*) dll_fflush,                    NULL },
  { "xbp_fwrite",                 -1, (void*) dll_fwrite,                    NULL },
  //{ "xbp_fprintf",                -1, (void*) xbp_fprintf,                   NULL },
  { "xbp_fread",                  -1, (void*) dll_fread,                     NULL },
  //{ "xbp_fputs",                  -1, (void*) xbp_fputs,                     NULL },
  { "xbp_getc",                   -1, (void*) dll_getc,                      NULL },
  { "xbp_setvbuf",                -1, (void*) setvbuf,                       NULL },
  { "xbp_fsetpos",                -1, (void*) dll_fsetpos,                   NULL },
  { "xbp_fgetpos",                -1, (void*) dll_fgetpos,                   NULL },
  { "xbp__lseek",                 -1, (void*) dll_lseek,                     NULL },
  { "xbp_ftell",                  -1, (void*) dll_ftell,                     NULL },
  //{ "xbp_ftell64",                -1, (void*) ftell64,                       NULL },
  { "xbp_fgets",                  -1, (void*) dll_fgets,                     NULL },
  { "xbp_fseek",                  -1, (void*) dll_fseek,                     NULL },
  //{ "xbp_putc",                   -1, (void*) xbp_putc,                      NULL },
  //{ "xbp_write",                  -1, (void*) xbp_write,                     NULL },
  { "xbp_read",                   -1, (void*) dll_read,                      NULL },
  { "xbp_close",                  -1, (void*) xbp_close,                     NULL },
  { "xbp_mkdir",                  -1, (void*) xbp_mkdir,                     NULL },
  { "xbp_open",                   -1, (void*) xbp_open,                      NULL },
  //{ "xbp__fdopen",                -1, (void*) xbp__fdopen,                   NULL },
  { "xbp_lseek",                  -1, (void*) dll_lseek,                     NULL },
  { "xbp_fstat",                  -1, (void*) dll_fstat,                     NULL },
  //{ "xbp_stat",                   -1, (void*) xbp_stat,                      NULL },
  { "xbp__lseeki64",              -1, (void*) dll_lseeki64,                     NULL },
  //{ "xbp_fileno",                 -1, (void*) xbp_fileno,                    NULL },
  //{ "xbp_fputc",                  -1, (void*) xbp_fputc,                     NULL },
  { "xbp_rewind",                 -1, (void*) rewind,                        NULL },
  { "xbp_fclose",                 -1, (void*) xbp_fclose,                    NULL },
  { "xbp_isatty",                 -1, (void*) isatty,                        NULL },
  { "xbp_getcwd",                 -1, (void*) xbp_getcwd,                    NULL },
  //{ "xbp_getenv",                 -1, (void*) xbp_getenv,                    NULL },
  //{ "xbp_putenv",                 -1, (void*) xbp_putenv,                    NULL },
  { "xbp_FindClose",              -1, (void*) xbp_FindClose,                 NULL },
  { "xbp_FindFirstFile",          -1, (void*) xbp_FindFirstFile,             NULL },
  { "xbp_FindNextFile",           -1, (void*) xbp_FindNextFile,              NULL },
  { NULL, -1, (void*) NULL, NULL }
};
