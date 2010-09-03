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
#include <dlfcn.h>

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
static FILE *(*libc_fopen)(const char*, const char*);

/* Values to set/unset libc function interposing. */
static int interpose_fopen;

/* functions defined in emu_msvcrt.cpp */
FILE * dll_fopen(const char * filename, const char * mode);

/* Function to initialize dlopen and dlsym handles. It should be called before
 * using any overridden libc function.
 */
extern void xbmc_libc_init(void);
void xbmc_libc_init_nlf(void)
{
  /* enable libc callbacks via dlopen */
  if (dlopen_handle) return;
  dlopen_handle = dlopen(C_LIB, RTLD_LOCAL | RTLD_LAZY);
  libc_fopen = dlsym(dlopen_handle, "fopen");
}

/* This function sets/unsets function interposing. To enable/disable some
 * function to be interposed, set the correct enum value for 'func' and set mode
 * to 0 for diabling interposing, and some non-zero value to enable interposing.
 */
int xbmc_libc_interpose_nlf(int func, int mode)
{
  switch (func)
  {
    case XBMC_LIBC_FOPEN:
      interpose_fopen = mode;
      return 1;
  }
  return 0;
}

/* Shorthand function to set all functions to be interposed */
void xbmc_libc_interpose_all_nlf(int mode)
{
  interpose_fopen = mode;
}

FILE *fopen(const char *path, const char *mode)
{
  /* fopen is called before anything via libGL, so ensure the libc function
   * overrides are ready.
   */
  if (!dlopen_handle)
    xbmc_libc_init();

  if (!interpose_fopen)
    return (*libc_fopen)(path, mode);
  return dll_fopen(path, mode);
}
