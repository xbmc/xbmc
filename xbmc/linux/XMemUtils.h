#ifndef __XMEMUTILS__H__
#define __XMEMUTILS__H__

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "linux/PlatformDefs.h"

// aligned memory allocation and free. memory returned will be aligned to "alignTo" bytes.
// this is a linux (actually platfom free) implementation of the win32 CRT methods _aligned_malloc and _aligned_free.
void *_aligned_malloc(size_t s, size_t alignTo);
void _aligned_free(void *p) ;

void GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer);

#endif

