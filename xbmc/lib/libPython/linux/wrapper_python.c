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
#include <pthread.h>
#include <dlfcn.h>
#include <limits.h>

/* The names of libc libraries for different systems */
#if defined(_WIN32)
#  define C_LIB "msvcr90.dll"
#elif defined(__APPLE__)
#  define C_LIB "libc.dylib"
#else
#  define C_LIB "libc.so.6"
#endif

/* These are attributes that enable xbmc_libc_init to be automatically called
 * before entering main() or during library initilization.
 * Idea for the MSVC declaration is from
 * http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/auxiliary/util/u_init.h
 */
#if defined(_MSC_VER)
#  define XBMC_LIBC_INIT \
   static void __cdecl xbmc_libc_init(void); \
   __declspec(allocate(".CRT$XCU")) void (__cdecl* xbmc_libc_init__xcu)(void) = xbmc_libc_init; \
   static void __cdecl xbmc_libc_init(void)
#elif defined(__GNUC__)
#  define XBMC_LIBC_INIT __attribute__((constructor)) static void xbmc_libc_init(void)
#else
#  define XBMC_LIBC_INIT void xbmc_libc_init(void)
#endif

/* Global dlopen handle */
static void *dlopen_handle;

/* Global dlsym handles */
static char *(*libc_getcwd)(char*,size_t);
static int (*libc_chdir)(const char*);
static FILE *(*libc_fopen64)(const char*, const char*);

/* Function prototypes of overridden libc functions */
char *getcwd(char *buf, size_t size);
int chdir(const char *path);

/* Function to initialize dlopen and dlsym handles. It should be called before
 * using any overridden libc function.
 */
XBMC_LIBC_INIT
{
  dlopen_handle = dlopen(C_LIB, RTLD_LOCAL | RTLD_LAZY);
  libc_getcwd = dlsym(dlopen_handle, "getcwd");
  libc_chdir = dlsym(dlopen_handle, "chdir");
  libc_fopen64 = dlsym(dlopen_handle, "fopen64");
}

#ifdef __APPLE__
/* Use pthread's built-in support for TLS, it's more portable. */
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t  tWorkingDir = 0;

/* Called once and only once. */
static void MakeTlsKeys()
{
  pthread_key_create(&tWorkingDir, free);
}

#define xbp_cw_dir (char*)pthread_getspecific(tWorkingDir)

#else
__thread char xbp_cw_dir[PATH_MAX] = "";
#endif

/* Overridden functions */
char *getcwd(char *buf, size_t size)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);
  if (xbp_cw_dir == 0)
  {
    printf("Initializing Python path...\n");
    char* path = (char* )malloc(PATH_MAX);
    strcpy(path, _P("special://xbmc/system/python").c_str());
    pthread_setspecific(tWorkingDir, (void*)path);
  }
#endif

  if (buf == NULL) buf = (char *)malloc(size);
  strcpy(buf, xbp_cw_dir);
  return buf;
}

int chdir(const char *dirname)
{
#ifdef __APPLE__
  // Initialize thread local storage and local thread pointer.
  pthread_once(&keyOnce, MakeTlsKeys);

  if (xbp_cw_dir == 0)
  {
    char* path = (char* )malloc(PATH_MAX);
    strcpy(path, _P("special://xbmc/system/python").c_str());
    pthread_setspecific(tWorkingDir, (void*)path);
  }
#endif

  if (strlen(dirname) > PATH_MAX) return -1;
  strcpy(xbp_cw_dir, dirname);
  return 0;
}

FILE* fopen64(const char *filename, const char *mode)
{
#ifdef __APPLE__
  return fopen(filename, mode);
#else
  return (*libc_fopen64)(filename, mode);
#endif
}
