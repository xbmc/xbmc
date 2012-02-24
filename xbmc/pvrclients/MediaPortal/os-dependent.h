#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_WINDOWS)
#define NOMINMAX                      // don't define min() and max() to prevent a clash with std::min() and std::max
#endif

#include "platform/os.h"

#if defined(TARGET_WINDOWS)
#  include "windows/os_windows.h"
#  if defined LIVE555
   // Define needed for including headers from LIVE555 (see NetCommon.h)
#    ifndef _WIN32
#      define _WIN32
#    endif
#  endif
#else
#  include "posix/os_posix.h"
#endif

#if defined(TARGET_DARWIN)
#  ifndef PTHREAD_MUTEX_RECURSIVE_NP
#    define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#  endif
#endif
