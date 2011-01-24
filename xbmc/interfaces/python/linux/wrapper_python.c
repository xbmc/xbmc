/*
 *      Copyright (C) 2007-2010 Team XBMC
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <utime.h>
#include <fcntl.h>

char* xbp_getcwd(char *buf, int size);
int xbp_chdir(const char *dirname);
int xbp_access(const char *path, int mode);
int xbp_unlink(const char *filename);
int xbp_chmod(const char *filename, int pmode);
int xbp_rmdir(const char *dirname);
int xbp_utime(const char *filename, struct utimbuf *times);
int xbp_rename(const char *oldname, const char *newname);
int xbp_mkdir(const char *dirname);
int xbp_open(const char *filename, int oflag, int pmode);
int xbp_lstat(const char * path, struct stat * buf);
#ifndef __APPLE__
int xbp_lstat64(const char * path, struct stat64 * buf);
#endif
void *xbp_dlopen(const char *filename, int flag);
int xbp_dlclose(void *handle);
void *xbp_dlsym(void *handle, const char *symbol);

#define PYTHON_WRAP(func) __wrap_##func

int PYTHON_WRAP(access)(const char* path, int mode)
{
  return xbp_access(path, mode);
}

char* PYTHON_WRAP(getcwd)(char *buf, int size)
{
  return xbp_getcwd(buf, size);
}

int PYTHON_WRAP(chdir)(const char *dirname)
{
  return xbp_chdir(dirname);
}

int PYTHON_WRAP(unlink)(const char *filename)
{
  return xbp_unlink(filename);
}

int PYTHON_WRAP(chmod)(const char *filename, int pmode)
{
  return xbp_chmod(filename, pmode);
}

int PYTHON_WRAP(rmdir)(const char *dirname)
{
  return xbp_rmdir(dirname);
}

int PYTHON_WRAP(utime)(const char *filename, struct utimbuf *times)
{
  return xbp_utime(filename, times);
}

int PYTHON_WRAP(rename)(const char *oldname, const char *newname)
{
  return xbp_rename(oldname, newname);
}

int PYTHON_WRAP(mkdir)(const char *dirname)
{
  return xbp_mkdir(dirname);
}

int PYTHON_WRAP(lstat)(const char * path, struct stat * buf)
{
  return xbp_lstat(path, buf);
}
#ifndef __APPLE__
int PYTHON_WRAP(lstat64)(const char * path, struct stat64 * buf)
{
  return xbp_lstat64(path, buf);
}
#endif

void *PYTHON_WRAP(dlopen)(const char *filename, int flag)
{
  return xbp_dlopen(filename,flag);
}

int PYTHON_WRAP(dlclose)(void *handle)
{
  return xbp_dlclose(handle);
}

void *PYTHON_WRAP(dlsym)(void *handle, const char *symbol)
{
  return xbp_dlsym(handle, symbol);
}

