/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "smmintrin.h"

#define CACHED_BUFFER_SIZE 4096

extern "C"
{

/*
 * http://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers
 * COPIES VIDEO FRAMES FROM USWC MEMORY TO WB SYSTEM MEMORY VIA CACHED BUFFER
 * ASSUMES PITCH IS A MULTIPLE OF 64B CACHE LINE SIZE, WIDTH MAY NOT BE
 */
void copy_frame( void * pSrc, void * pDest, void * pCacheBlock,
    unsigned int width, unsigned int height, unsigned int pitch )
{
  __m128i         x0, x1, x2, x3;
  __m128i         *pLoad;
  __m128i         *pStore;
  __m128i         *pCache;
  unsigned int x, y, yLoad, yStore;
  unsigned int rowsPerBlock;
  unsigned int width64;
  unsigned int extraPitch;


  rowsPerBlock = CACHED_BUFFER_SIZE / pitch;
  width64 = (width + 63) & ~0x03f;
  extraPitch = (pitch - width64) / 16;

  pLoad  = (__m128i *)pSrc;
  pStore = (__m128i *)pDest;

  //  COPY THROUGH 4KB CACHED BUFFER
  for( y = 0; y < height; y += rowsPerBlock  )
  {
    //  ROWS LEFT TO COPY AT END
    if( y + rowsPerBlock > height )
      rowsPerBlock = height - y;

    pCache = (__m128i *)pCacheBlock;

    _mm_mfence();

    // LOAD ROWS OF PITCH WIDTH INTO CACHED BLOCK
    for( yLoad = 0; yLoad < rowsPerBlock; yLoad++ )
    {
      // COPY A ROW, CACHE LINE AT A TIME
      for( x = 0; x < pitch; x +=64 )
      {
        x0 = _mm_stream_load_si128( pLoad +0 );
        x1 = _mm_stream_load_si128( pLoad +1 );
        x2 = _mm_stream_load_si128( pLoad +2 );
        x3 = _mm_stream_load_si128( pLoad +3 );

        _mm_store_si128( pCache +0,     x0 );
        _mm_store_si128( pCache +1, x1 );
        _mm_store_si128( pCache +2, x2 );
        _mm_store_si128( pCache +3, x3 );

        pCache += 4;
        pLoad += 4;
      }
    }

    _mm_mfence();

    pCache = (__m128i *)pCacheBlock;

    // STORE ROWS OF FRAME WIDTH FROM CACHED BLOCK
    for( yStore = 0; yStore < rowsPerBlock; yStore++ )
    {
      // copy a row, cache line at a time
      for( x = 0; x < width64; x +=64 )
      {
        x0 = _mm_load_si128( pCache );
        x1 = _mm_load_si128( pCache +1 );
        x2 = _mm_load_si128( pCache +2 );
        x3 = _mm_load_si128( pCache +3 );

        _mm_stream_si128( pStore,       x0 );
        _mm_stream_si128( pStore +1, x1 );
        _mm_stream_si128( pStore +2, x2 );
        _mm_stream_si128( pStore +3, x3 );

        pCache += 4;
        pStore += 4;
      }

      pCache += extraPitch;
      pStore += extraPitch;
    }
  }
}
}
