/*
 * Copyright (C) 2000, 2001 Billy Biggs <vektor@dumbterm.net>,
 *                          Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBDVDREAD_BSWAP_H
#define LIBDVDREAD_BSWAP_H

#include <config.h>

#if defined(WORDS_BIGENDIAN)
/* All bigendian systems are fine, just ignore the swaps. */
#define B2N_16(x) (void)(x)
#define B2N_32(x) (void)(x)
#define B2N_64(x) (void)(x)

#else

/* For __FreeBSD_version */
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#if defined(__linux__) || defined(__GLIBC__)
#include <byteswap.h>
#define B2N_16(x) x = bswap_16(x)
#define B2N_32(x) x = bswap_32(x)
#define B2N_64(x) x = bswap_64(x)

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define B2N_16(x) x = OSSwapBigToHostInt16(x)
#define B2N_32(x) x = OSSwapBigToHostInt32(x)
#define B2N_64(x) x = OSSwapBigToHostInt64(x)

#elif defined(__NetBSD__)
#include <sys/endian.h>
#define B2N_16(x) BE16TOH(x)
#define B2N_32(x) BE32TOH(x)
#define B2N_64(x) BE64TOH(x)

#elif defined(__OpenBSD__)
#include <sys/endian.h>
#define B2N_16(x) x = swap16(x)
#define B2N_32(x) x = swap32(x)
#define B2N_64(x) x = swap64(x)

#elif defined(__FreeBSD__) && __FreeBSD_version >= 470000
#include <sys/endian.h>
#define B2N_16(x) x = be16toh(x)
#define B2N_32(x) x = be32toh(x)
#define B2N_64(x) x = be64toh(x)

/* This is a slow but portable implementation, it has multiple evaluation
 * problems so beware.
 * Old FreeBSD's and Solaris don't have <byteswap.h> or any other such
 * functionality!
 */

#elif defined(__FreeBSD__) || defined(__sun) || defined(__bsdi__) || defined(WIN32) || defined(__CYGWIN__) || defined(__BEOS__) || defined(__OS2__)
#define B2N_16(x)                             \
 x = ((((x) & 0xff00) >> 8) |                 \
      (((x) & 0x00ff) << 8))
#define B2N_32(x)                             \
 x = ((((x) & 0xff000000) >> 24) |            \
      (((x) & 0x00ff0000) >>  8) |            \
      (((x) & 0x0000ff00) <<  8) |            \
      (((x) & 0x000000ff) << 24))
#define B2N_64(x)                             \
 x = ((((x) & 0xff00000000000000ULL) >> 56) | \
      (((x) & 0x00ff000000000000ULL) >> 40) | \
      (((x) & 0x0000ff0000000000ULL) >> 24) | \
      (((x) & 0x000000ff00000000ULL) >>  8) | \
      (((x) & 0x00000000ff000000ULL) <<  8) | \
      (((x) & 0x0000000000ff0000ULL) << 24) | \
      (((x) & 0x000000000000ff00ULL) << 40) | \
      (((x) & 0x00000000000000ffULL) << 56))

#else

/* If there isn't a header provided with your system with this functionality
 * add the relevant || define( ) to the portable implementation above.
 */
#error "You need to add endian swap macros for you're system"

#endif

#endif /* WORDS_BIGENDIAN */

#endif /* LIBDVDREAD_BSWAP_H */
