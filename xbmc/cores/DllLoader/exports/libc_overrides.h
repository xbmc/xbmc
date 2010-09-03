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

#ifndef LIBC_OVERRIDES_H
#define LIBC_OVERRIDES_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  XBMC_LIBC_OPEN,
  XBMC_LIBC_OPEN64,
  XBMC_LIBC_FOPEN,
  XBMC_LIBC_FOPEN64,
  XBMC_LIBC_FDOPEN,
  XBMC_LIBC_FREOPEN,
  XBMC_LIBC_GETCWD,
};

#if !(defined _WIN32 && defined __APPLE__)
/* function used to enable/disable libc function interpose */
extern int xbmc_libc_interpose(int func, int mode);
extern void xbmc_libc_interpose_all(int mode);
#else
#define xbmc_libc_interpose(a, b)
#define xbmc_libc_interpose_all(a)
#endif

#ifdef __cplusplus
}
#endif

#endif
