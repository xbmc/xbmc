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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#else
  #ifdef WIN32
    #define USE_WIN_THREADING 1
  #endif
#endif

#ifdef USE_PTHREADS_THREADING
  #define XBMC_THREADING_IMPL_SET 1
#endif

#ifdef USE_WIN_THREADING
  #ifdef XBMC_THREADING_IMPL_SET
    #error "Cannot define both USE_WIN_THREADING and another USE_*_THREADING"
  #endif
  #define XBMC_THREADING_IMPL_SET 1
#endif

#ifndef XBMC_THREADING_IMPL_SET
  #error "No threading implementation selected."
#endif

#undef XBMC_THREADING_IMPL_SET
