/*
    $Id$

    Copyright (C) 2000, 2004 Herbert Valerio Riedel <hvr@gnu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __CDIO_BYTESEX_H__
#define __CDIO_BYTESEX_H__

#include <types.h>
#include <bytesex_asm.h>
#include <logging.h>

/* generic byteswap routines */

#define UINT16_SWAP_LE_BE_C(val) ((uint16_t) ( \
    (((uint16_t) (val) & (uint16_t) 0x00ffU) << 8) | \
    (((uint16_t) (val) & (uint16_t) 0xff00U) >> 8)))

#define UINT32_SWAP_LE_BE_C(val) ((uint32_t) ( \
    (((uint32_t) (val) & (uint32_t) 0x000000ffU) << 24) | \
    (((uint32_t) (val) & (uint32_t) 0x0000ff00U) <<  8) | \
    (((uint32_t) (val) & (uint32_t) 0x00ff0000U) >>  8) | \
    (((uint32_t) (val) & (uint32_t) 0xff000000U) >> 24)))

#define UINT64_SWAP_LE_BE_C(val) ((uint64_t) ( \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x00000000000000ff)) << 56) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x000000000000ff00)) << 40) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x0000000000ff0000)) << 24) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x00000000ff000000)) <<  8) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x000000ff00000000)) >>  8) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x0000ff0000000000)) >> 24) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0x00ff000000000000)) >> 40) | \
    (((uint64_t) (val) & (uint64_t) UINT64_C(0xff00000000000000)) >> 56)))

#ifndef UINT16_SWAP_LE_BE
# define UINT16_SWAP_LE_BE UINT16_SWAP_LE_BE_C
#endif

#ifndef UINT32_SWAP_LE_BE
# define UINT32_SWAP_LE_BE UINT32_SWAP_LE_BE_C
#endif

#ifndef UINT64_SWAP_LE_BE
# define UINT64_SWAP_LE_BE UINT64_SWAP_LE_BE_C
#endif

inline static 
uint16_t uint16_swap_le_be (const uint16_t val)
{
  return UINT16_SWAP_LE_BE (val);
}

inline static 
uint32_t uint32_swap_le_be (const uint32_t val)
{
  return UINT32_SWAP_LE_BE (val);
}

inline static 
uint64_t uint64_swap_le_be (const uint64_t val)
{
  return UINT64_SWAP_LE_BE (val);
}

# define UINT8_TO_BE(val)      ((uint8_t) (val))
# define UINT8_TO_LE(val)      ((uint8_t) (val))
#ifdef WORDS_BIGENDIAN
# define UINT16_TO_BE(val)     ((uint16_t) (val))
# define UINT16_TO_LE(val)     ((uint16_t) UINT16_SWAP_LE_BE(val))

# define UINT32_TO_BE(val)     ((uint32_t) (val))
# define UINT32_TO_LE(val)     ((uint32_t) UINT32_SWAP_LE_BE(val))

# define UINT64_TO_BE(val)     ((uint64_t) (val))
# define UINT64_TO_LE(val)     ((uint64_t) UINT64_SWAP_LE_BE(val))
#else
# define UINT16_TO_BE(val)     ((uint16_t) UINT16_SWAP_LE_BE(val))
# define UINT16_TO_LE(val)     ((uint16_t) (val))

# define UINT32_TO_BE(val)     ((uint32_t) UINT32_SWAP_LE_BE(val))
# define UINT32_TO_LE(val)     ((uint32_t) (val))

# define UINT64_TO_BE(val)     ((uint64_t) UINT64_SWAP_LE_BE(val))
# define UINT64_TO_LE(val)     ((uint64_t) (val))
#endif

/* symmetric conversions */
#define UINT8_FROM_BE(val)     (UINT8_TO_BE (val))
#define UINT8_FROM_LE(val)     (UINT8_TO_LE (val))
#define UINT16_FROM_BE(val)    (UINT16_TO_BE (val))
#define UINT16_FROM_LE(val)    (UINT16_TO_LE (val))
#define UINT32_FROM_BE(val)    (UINT32_TO_BE (val))
#define UINT32_FROM_LE(val)    (UINT32_TO_LE (val))
#define UINT64_FROM_BE(val)    (UINT64_TO_BE (val))
#define UINT64_FROM_LE(val)    (UINT64_TO_LE (val))

/* converter function template */
#define CVT_TO_FUNC(bits) \
 static inline uint ## bits ## _t \
 uint ## bits ## _to_be (uint ## bits ## _t val) \
 { return UINT ## bits ## _TO_BE (val); } \
 static inline uint ## bits ## _t \
 uint ## bits ## _to_le (uint ## bits ## _t val) \
 { return UINT ## bits ## _TO_LE (val); } \

CVT_TO_FUNC(8)
CVT_TO_FUNC(16)
CVT_TO_FUNC(32)
CVT_TO_FUNC(64)

#undef CVT_TO_FUNC

#define uint8_from_be(val)     (uint8_to_be (val))
#define uint8_from_le(val)     (uint8_to_le (val))
#define uint16_from_be(val)    (uint16_to_be (val))
#define uint16_from_le(val)    (uint16_to_le (val))
#define uint32_from_be(val)    (uint32_to_be (val))
#define uint32_from_le(val)    (uint32_to_le (val))
#define uint64_from_be(val)    (uint64_to_be (val))
#define uint64_from_le(val)    (uint64_to_le (val))

/* ISO9660 related stuff */

#define to_711(i)   uint8_to_le(i)
#define from_711(i) uint8_from_le(i)

#define to_721(i)   uint16_to_le(i)
#define from_721(i) uint16_from_le(i)

#define to_721(i)   uint16_to_le(i)
#define from_721(i) uint16_from_le(i)

#define to_722(i)   uint16_to_be(i)
#define from_722(i) uint16_from_be(i)

static inline uint32_t
to_723(uint16_t i)
{
  return uint32_swap_le_be(i) | i;
}

static inline uint16_t 
from_723 (uint32_t p)
{
  if (uint32_swap_le_be (p) != p)
    cdio_warn ("from_723: broken byte order");

  return (0xFFFF & p);
}

#define to_731(i)   uint32_to_le(i)
#define from_731(i) uint32_from_le(i)

#define to_732(i)   uint32_to_be(i)
#define from_732(i) uint32_from_be(i)

static inline uint64_t
to_733(uint32_t i)
{
  return uint64_swap_le_be(i) | i;
}

static inline uint32_t 
from_733 (uint64_t p)
{
  if (uint64_swap_le_be (p) != p)
    cdio_warn ("from_733: broken byte order");
    
  return (UINT32_C(0xFFFFFFFF) & p);
}

#endif /* __CDIO_BYTESEX_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
