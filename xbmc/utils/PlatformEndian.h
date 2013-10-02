/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
#pragma once

#ifdef TARGET_WINDOWS
  /* All versions of Windows are little-endian */ 
  #define TARGET_LITTLEENDIAN 1
#else // ! TARGET_WINDOWS

#ifdef HAVE_CONFIG_H
  #include "config.h"
  #if defined(HAVE_ENDIAN_H)
    #include <endian.h>
  #elif defined (HAVE_SYS_PARAM_H)
    #include <sys/param.h>
  #endif
#endif

// check system headers macros
#if (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN)
#define TARGET_BIGENDIAN 1

#elif (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN) || \
      (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && BYTE_ORDER == LITTLE_ENDIAN)
#define TARGET_LITTLEENDIAN 1

// check predefined macros
#elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
  defined(__ARMEB__) || defined (__THUMBEB__) || defined (__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#define TARGET_BIGENDIAN 1

#elif defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
  defined(__amd64__) || defined(__amd64) || defined(__x86_64__) ||  defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64) || \
  defined(i386) || defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86) || \
  defined(_X86_) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) ||  defined(_MIPSEL) || defined(__MIPSEL) || \
  defined(__MIPSEL__)
#define TARGET_LITTLEENDIAN 1

// detection by "configure" can be inaccurate, use it only if other methods failed
#elif defined(WORDS_BIGENDIAN)
#define TARGET_BIGENDIAN 1
#else
#define TARGET_LITTLEENDIAN 1

#endif
#endif // ! TARGET_WINDOWS

#ifndef BIG_ENDIAN 
  #define BIG_ENDIAN 4321
#endif

#ifndef LITTLE_ENDIAN
  #define LITTLE_ENDIAN 1234
#endif

#ifdef TARGET_BIGENDIAN
  #define PLATFROM_ENDIANNESS BIG_ENDIAN
#else
  #define PLATFROM_ENDIANNESS LITTLE_ENDIAN
#endif

