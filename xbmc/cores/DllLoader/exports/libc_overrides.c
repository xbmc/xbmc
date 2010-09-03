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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdarg.h>

#include "libc_overrides.h"

/* The names of libc libraries for different systems */
#if defined(_WIN32)
#  define C_LIB "msvcr90.dll"
#elif defined(__APPLE__)
#  define C_LIB "libc.dylib"
#else
#  define C_LIB "libc.so.6"
#endif

/* dlopen handle */
static void *dlopen_handle;

/* dlsym handles */
static int (*libc_open)(const char*, int, ...);
static int (*libc_open64)(const char*, int, ...);
static FILE *(*libc_fopen64)(const char*, const char*);
static FILE *(*libc_fdopen)(int, const char*);
static FILE *(*libc_freopen)(const char*, const char*, FILE*);
static char *(*libc_getcwd)(char *buf, size_t size);

/* Values to set/unset libc function interposing. */
static int interpose_open;
static int interpose_open64;
static int interpose_fopen64;
static int interpose_fdopen;
static int interpose_freopen;
static int interpose_getcwd;

/* functions defined in emu_msvcrt.cpp */
int dll_open(const char* szFileName, int iMode);
FILE * dll_fopen(const char * filename, const char * mode);
FILE* dll_freopen(const char *path, const char *mode, FILE *stream);
FILE* dll_fdopen(int i, const char* file);
char *dll_getcwd(char *buf, size_t size);
int dll_chdir(const char *dirname);

/* Function to initialize dlopen and dlsym handles. It should be called before
 * using any overridden libc function.
 */
extern void xbmc_libc_init_nlf(void);
void xbmc_libc_init(void)
{
  /* enable libc callbacks via dlopen */
  if (dlopen_handle) return;
  dlopen_handle = dlopen(C_LIB, RTLD_LOCAL | RTLD_LAZY);
  libc_open = dlsym(dlopen_handle, "open");
  libc_open64 = dlsym(dlopen_handle, "open64");
  libc_fopen64 = dlsym(dlopen_handle, "fopen64");
  libc_fdopen = dlsym(dlopen_handle, "fdopen");
  libc_freopen = dlsym(dlopen_handle, "freopen");
  libc_getcwd = dlsym(dlopen_handle, "getcwd");
  xbmc_libc_init_nlf();
}

/* This function sets/unsets function interposing. To enable/disable some
 * function to be interposed, set the correct enum value for 'func' and set mode
 * to 0 for diabling interposing, and some non-zero value to enable interposing.
 */
extern int xbmc_libc_interpose_nlf(int func, int mode);
int xbmc_libc_interpose(int func, int mode)
{
  switch (func)
  {
    case XBMC_LIBC_OPEN:
      interpose_open = mode;
      return 1;
    case XBMC_LIBC_OPEN64:
      interpose_open64 = mode;
      return 1;
    case XBMC_LIBC_FOPEN:
      return xbmc_libc_interpose_nlf(func, mode);
    case XBMC_LIBC_FOPEN64:
      interpose_fopen64 = mode;
      return 1;
    case XBMC_LIBC_FDOPEN:
      interpose_fdopen = mode;
      return 1;
    case XBMC_LIBC_FREOPEN:
      interpose_freopen = mode;
      return 1;
    case XBMC_LIBC_GETCWD:
      interpose_getcwd = mode;
      return 1;
  }
  return 0;
}

/* Shorthand function to enable/disable all functions to be interposed */
extern void xbmc_libc_interpose_all_nlf(int mode);
void xbmc_libc_interpose_all(int mode)
{
  interpose_open = mode;
  interpose_open64 = mode;
  interpose_fopen64 = mode;
  interpose_fdopen = mode;
  interpose_freopen = mode;
  interpose_getcwd = mode;
  xbmc_libc_interpose_all_nlf(mode);
}

/* Overridden functions */
int open(const char *file, int oflag, ...)
{
  if (!interpose_open)
  {
    int res;
    va_list va;
    va_start(va, oflag);
    res = (*libc_open)(file, oflag, va);
    va_end(va);
    return res;
  }
  return dll_open(file, oflag);
}

int open64(const char *file, int oflag, ...)
{
  if (!interpose_open64)
  {
    int res;
    va_list va;
    va_start(va, oflag);
    res = (*libc_open64)(file, oflag, va);
    va_end(va);
    return res;
  }
  return dll_open(file, oflag);
}

FILE *fopen64(const char *path, const char *mode)
{
  if (!interpose_fopen64)
    return (*libc_fopen64)(path, mode);
  return dll_fopen(path, mode);
}

FILE *fdopen(int fildes, const char *mode)
{
  if (!interpose_fdopen)
    return (*libc_fdopen)(fildes, mode);
  return dll_fdopen(fildes, mode);
}

FILE *freopen(const char *path, const char *mode, FILE *stream)
{
  if (!interpose_freopen)
    return (*libc_freopen)(path, mode, stream);
  return dll_freopen(path, mode, stream);
}

char *getcwd(char *buf, size_t size)
{
  if (!interpose_getcwd)
    return (*libc_getcwd)(buf, size);
  return dll_getcwd(buf, size);
}

int chdir(const char *dirname)
{
  return dll_chdir(dirname);
}
