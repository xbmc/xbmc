/* lzo1x.h -- public interface of the LZO1X compression algorithm

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   markus.oberhumer@jk.uni-linz.ac.at
 */


#ifndef __LZO1X_H
#define __LZO1X_H

#ifndef __LZOCONF_H
#include "lzoconf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

/* Memory required for the wrkmem parameter.
 * When the required size is 0, you can also pass a NULL pointer.
 */

#define LZO1X_MEM_COMPRESS      ((lzo_uint) (16384L * sizeof(lzo_byte *)))
#define LZO1X_MEM_DECOMPRESS    (0)
#define LZO1X_MEM_OPTIMIZE      (0)


/* decompression */
LZO_EXTERN(int)
lzo1x_decompress        ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem /* NOT USED */ );

/* safe decompression with overrun testing */
LZO_EXTERN(int)
lzo1x_decompress_safe   ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem /* NOT USED */ );

/***********************************************************************
//
************************************************************************/

LZO_EXTERN(int)
lzo1x_1_compress        ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem );


/***********************************************************************
// special compressor versions
************************************************************************/

/* this version needs only 8kB work memory */
#define LZO1X_1_11_MEM_COMPRESS ((lzo_uint) (2048L * sizeof(lzo_byte *)))

LZO_EXTERN(int)
lzo1x_1_11_compress     ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem );


/* use this version if you need a little more compression speed */
#define LZO1X_1_15_MEM_COMPRESS ((lzo_uint) (32768L * sizeof(lzo_byte *)))

LZO_EXTERN(int)
lzo1x_1_15_compress     ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem );


/***********************************************************************
// better compression ratio at the cost of more memory and time
************************************************************************/

#define LZO1X_999_MEM_COMPRESS  ((lzo_uint) (14 * 16384L * sizeof(short)))

#if !defined(LZO_999_UNSUPPORTED)
LZO_EXTERN(int)
lzo1x_999_compress      ( const lzo_byte *src, lzo_uint  src_len,
                                lzo_byte *dst, lzo_uint *dst_len,
                                lzo_voidp wrkmem );
#endif


/***********************************************************************
// optimize a compressed data block
************************************************************************/

LZO_EXTERN(int)
lzo1x_optimize          (       lzo_byte *in , lzo_uint  in_len,
                                lzo_byte *out, lzo_uint *out_len,
                                lzo_voidp wrkmem );



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */

