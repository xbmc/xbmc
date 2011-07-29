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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#ifdef TARGET_POSIX
  #define XBMC_THREADING_IMPL_SET 1
  #define USE_PTHREADS_THREADING
#endif

#ifdef TARGET_WINDOWS
  #ifdef XBMC_THREADING_IMPL_SET
    #error "Cannot define both TARGET_WINDOWS and TARGET_POSIX"
  #endif
  #define XBMC_THREADING_IMPL_SET 1
  #define USE_WIN_THREADING
#endif

#ifndef XBMC_THREADING_IMPL_SET
  #error "No threading implementation selected. You should be compiling with either TARGET_POSIX or TARGET_WINDOWS (or you will need to add another implementation to the threading model)."
#endif

#undef XBMC_THREADING_IMPL_SET
