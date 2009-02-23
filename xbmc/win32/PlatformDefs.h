#ifndef __PLATFORM_DEFS_H__
#define __PLATFORM_DEFS_H__

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

#ifdef _WIN32

#define __STDC_FORMAT_MACROS
#include "inttypes.h"

typedef __int64 off64_t;
typedef __int64 fpos64_t;
typedef __int64 __off64_t;
typedef long    __off_t;

#define snprintf _snprintf
#define ftello64 _ftelli64
#define fseeko64 _fseeki64

#ifndef PRIdS
#define PRIdS "Id"
#endif

#define popen   _popen
#define pclose  _pclose 

#endif // _WIN32

#endif //__PLATFORM_DEFS_H__

