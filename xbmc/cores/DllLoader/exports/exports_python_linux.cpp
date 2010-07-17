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
#include <sys/types.h>
#include <utime.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dlfcn.h>

#include "../../../FileSystem/SpecialProtocol.h"
#include "../../../utils/log.h"
#ifdef HAS_PYTHON
#include "../../../lib/libPython/XBPython.h"
#endif
#include "../DllLoaderContainer.h"

#ifdef __APPLE__
//
// Use pthread's built-in support for TLS, it's more portable.
//
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t  tWorkingDir = 0;

//
// Called once and only once.
//
static void MakeTlsKeys()
{
  pthread_key_create(&tWorkingDir, free);
}

#define xbp_cw_dir (char* )pthread_getspecific(tWorkingDir)

#else
__thread char xbp_cw_dir[MAX_PATH] = "";
#endif

extern "C"
{

void *xbp_dlopen(const char *filename, int flag)
{
#ifdef HAS_PYTHON
  CLog::Log(LOGDEBUG,"%s loading python lib %s. flags: %d", __FUNCTION__, filename, flag);
  LibraryLoader* pDll = DllLoaderContainer::LoadModule(filename);
  if (pDll)
  {
    g_pythonParser.RegisterExtensionLib(pDll);
  }
  else
  {
    CLog::Log(LOGERROR,"%s failed to load %s", __FUNCTION__, filename);
  }
  return pDll;
#else
  CLog::Log(LOGDEBUG,"Cannot open python lib because python isnt being used!");
  return NULL;
#endif
}

int xbp_dlclose(void *handle)
{
#ifdef HAS_PYTHON
  LibraryLoader *pDll = (LibraryLoader*)handle;
  CLog::Log(LOGDEBUG,"%s - releasing python library %s", __FUNCTION__, pDll->GetName());
  g_pythonParser.UnregisterExtensionLib(pDll);
  DllLoaderContainer::ReleaseModule(pDll);
  return 0;
#else
  CLog::Log(LOGDEBUG,"Cannot release python lib because python isnt being used!");
  return NULL;
#endif
}

void *xbp_dlsym(void *handle, const char *symbol)
{
  LibraryLoader *pDll = (LibraryLoader*)handle;
  CLog::Log(LOGDEBUG,"%s - load symbol %s", __FUNCTION__, symbol);
  void *f = NULL;
  if (pDll && pDll->ResolveExport(symbol, &f))
    return f;

  return NULL;
}

char* xbp_getcwd(char *buf, int size)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);
  if (xbp_cw_dir == 0)
  {
    printf("Initializing Python path...\n");
    char* path = (char* )malloc(MAX_PATH);
    strcpy(path, _P("special://xbmc/system/python").c_str());
    pthread_setspecific(tWorkingDir, (void*)path);
  }
#endif

  if (buf == NULL) buf = (char *)malloc(size);
  strcpy(buf, xbp_cw_dir);
  return buf;
}

int xbp_chdir(const char *dirname)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);

  if (xbp_cw_dir == 0)
  {
    char* path = (char* )malloc(MAX_PATH);
    strcpy(path, _P("special://xbmc/system/python").c_str());
    pthread_setspecific(tWorkingDir, (void*)path);
  }
#endif

  if (strlen(dirname) > MAX_PATH) return -1;
  strcpy(xbp_cw_dir, dirname);

#if (defined USE_EXTERNAL_PYTHON)
  /* TODO: Need to figure out how to make system level Python make call to
   * XBMC's chdir instead of non-threadsafe system chdir
   */
  CStdString strName = _P(dirname);
  return chdir(strName.c_str());
#else
  return 0;
#endif
}

int xbp_unlink(const char *filename)
{
  CStdString strName = _P(filename);
  return unlink(strName.c_str());
}

int xbp_access(const char *path, int mode)
{
  CStdString strName = _P(path);
  return access(strName.c_str(), mode);
}

int xbp_chmod(const char *filename, int pmode)
{
  CStdString strName = _P(filename);
  return chmod(strName.c_str(), pmode);
}

int xbp_rmdir(const char *dirname)
{
  CStdString strName = _P(dirname);
  return rmdir(strName.c_str());
}

int xbp_utime(const char *filename, struct utimbuf *times)
{
  CStdString strName = _P(filename);
  return utime(strName.c_str(), times);
}

int xbp_rename(const char *oldname, const char *newname)
{
  CStdString strOldName = _P(oldname);
  CStdString strNewName = _P(newname);
  return rename(strOldName.c_str(), strNewName.c_str());
}

int xbp_mkdir(const char *dirname)
{
  CStdString strName = _P(dirname);
/*
  // If the dir already exists, don't try to recreate it
  struct stat buf;
  if (stat(strName.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode))
    return 0;
*/
  return mkdir(strName.c_str(), 0755);
}

int xbp_open(const char *filename, int oflag, int pmode)
{
  CStdString strName = _P(filename);
  return open(strName.c_str(), oflag, pmode);
}

FILE* xbp_fopen(const char *filename, const char *mode)
{
  CStdString strName = _P(filename);
  return fopen(strName.c_str(), mode);
}

FILE* xbp_freopen(const char *path, const char *mode, FILE *stream)
{
  CStdString strName = _P(path);
  return freopen(strName.c_str(), mode, stream);
}

FILE* xbp_fopen64(const char *filename, const char *mode)
{
  CStdString strName = _P(filename);
#ifdef __APPLE__
  return fopen(strName.c_str(), mode);
#else
  return fopen64(strName.c_str(), mode);
#endif
}

int xbp_lstat(const char * path, struct stat * buf)
{
  CStdString strName = _P(path);
  return lstat(strName.c_str(), buf);
}

#ifndef __APPLE__
int xbp_lstat64(const char * path, struct stat64 * buf)
{
  CStdString strName = _P(path);
  return lstat64(strName.c_str(), buf);
}
#endif

} // extern "C"
