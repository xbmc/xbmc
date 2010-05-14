/*
 * decode_i396.c: Mpeg Layer-1,2,3 audio decoder
 *
 * Copyright (C) 1999-2010 The L.A.M.E. project
 *
 * Initially written by Michael Hipp, see also AUTHORS and README.
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *
 * Slighlty optimized for machines without autoincrement/decrement.
 * The performance is highly compiler dependend. Maybe
 * the decode.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

/* $Id: decode_i386.c,v 1.18.2.2 2010/03/22 14:17:14 robert Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if defined(__riscos__) && defined(FPA10)
#include "ymath.h"
#else
#include <math.h>
#endif

#include "decode_i386.h"
#include "dct64_i386.h"
#include "tabinit.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


 /* old WRITE_SAMPLE_CLIPPED */
#define WRITE_SAMPLE_CLIPPED(TYPE,samples,sum,clip) \
  if( (sum) > 32767.0) { *(samples) = 0x7fff; (clip)++; } \
  else if( (sum) < -32768.0) { *(samples) = -0x8000; (clip)++; } \
  else { *(samples) = (TYPE)((sum)>0 ? (sum)+0.5 : (sum)-0.5) ; }

#define WRITE_SAMPLE_UNCLIPPED(TYPE,samples,sum,clip) \
  *samples = (TYPE)sum;

  /* *INDENT-OFF* */

 /* versions: clipped (when TYPE == short) and unclipped (when TYPE == real) of synth_1to1_mono* functions */
#define SYNTH_1TO1_MONO_CLIPCHOICE(TYPE,SYNTH_1TO1)                    \
  TYPE samples_tmp[64];                                                \
  TYPE *tmp1 = samples_tmp;                                            \
  int i,ret;                                                           \
  int pnt1 = 0;                                                        \
                                                                       \
  ret = SYNTH_1TO1 (mp,bandPtr,0,(unsigned char *) samples_tmp,&pnt1); \
  out += *pnt;                                                         \
                                                                       \
  for(i=0;i<32;i++) {                                                  \
    *( (TYPE *) out) = *tmp1;                                          \
    out += sizeof(TYPE);                                               \
    tmp1 += 2;                                                         \
  }                                                                    \
  *pnt += 32*sizeof(TYPE);                                             \
                                                                       \
  return ret; 

  /* *INDENT-ON* */


int
synth_1to1_mono(PMPSTR mp, real * bandPtr, unsigned char *out, int *pnt)
{
    SYNTH_1TO1_MONO_CLIPCHOICE(short, synth_1to1)
} int
synth_1to1_mono_unclipped(PMPSTR mp, real * bandPtr, unsigned char *out, int *pnt)
{
    SYNTH_1TO1_MONO_CLIPCHOICE(real, synth_1to1_unclipped)
}

    /* *INDENT-OFF* */
/* versions: clipped (when TYPE == short) and unclipped (when TYPE == real) of synth_1to1* functions */
#define SYNTH_1TO1_CLIPCHOICE(TYPE,WRITE_SAMPLE)         \
  static const int step = 2;                             \
  int bo;                                                \
  TYPE *samples = (TYPE *) (out + *pnt);                 \
                                                         \
  real *b0,(*buf)[0x110];                                \
  int clip = 0;                                          \
  int bo1;                                               \
                                                         \
  bo = mp->synth_bo;                                     \
                                                         \
  if(!channel) {                                         \
    bo--;                                                \
    bo &= 0xf;                                           \
    buf = mp->synth_buffs[0];                            \
  }                                                      \
  else {                                                 \
    samples++;                                           \
    buf = mp->synth_buffs[1];                            \
  }                                                      \
                                                         \
  if(bo & 0x1) {                                         \
    b0 = buf[0];                                         \
    bo1 = bo;                                            \
    dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);        \
  }                                                      \
  else {                                                 \
    b0 = buf[1];                                         \
    bo1 = bo+1;                                          \
    dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);                \
  }                                                      \
                                                         \
  mp->synth_bo = bo;                                     \
                                                         \
  {                                                      \
    int j;                                               \
    real *window = decwin + 16 - bo1;                    \
                                                         \
    for (j=16;j;j--,b0+=0x10,window+=0x20,samples+=step) \
    {                                                    \
      real sum;                                          \
      sum  = window[0x0] * b0[0x0];                      \
      sum -= window[0x1] * b0[0x1];                      \
      sum += window[0x2] * b0[0x2];                      \
      sum -= window[0x3] * b0[0x3];                      \
      sum += window[0x4] * b0[0x4];                      \
      sum -= window[0x5] * b0[0x5];                      \
      sum += window[0x6] * b0[0x6];                      \
      sum -= window[0x7] * b0[0x7];                      \
      sum += window[0x8] * b0[0x8];                      \
      sum -= window[0x9] * b0[0x9];                      \
      sum += window[0xA] * b0[0xA];                      \
      sum -= window[0xB] * b0[0xB];                      \
      sum += window[0xC] * b0[0xC];                      \
      sum -= window[0xD] * b0[0xD];                      \
      sum += window[0xE] * b0[0xE];                      \
      sum -= window[0xF] * b0[0xF];                      \
                                                         \
      WRITE_SAMPLE (TYPE,samples,sum,clip);              \
    }                                                    \
                                                         \
    {                                                    \
      real sum;                                          \
      sum  = window[0x0] * b0[0x0];                      \
      sum += window[0x2] * b0[0x2];                      \
      sum += window[0x4] * b0[0x4];                      \
      sum += window[0x6] * b0[0x6];                      \
      sum += window[0x8] * b0[0x8];                      \
      sum += window[0xA] * b0[0xA];                      \
      sum += window[0xC] * b0[0xC];                      \
      sum += window[0xE] * b0[0xE];                      \
      WRITE_SAMPLE (TYPE,samples,sum,clip);              \
      b0-=0x10,window-=0x20,samples+=step;               \
    }                                                    \
    window += bo1<<1;                                    \
                                                         \
    for (j=15;j;j--,b0-=0x10,window-=0x20,samples+=step) \
    {                                                    \
      real sum;                                          \
      sum = -window[-0x1] * b0[0x0];                     \
      sum -= window[-0x2] * b0[0x1];                     \
      sum -= window[-0x3] * b0[0x2];                     \
      sum -= window[-0x4] * b0[0x3];                     \
      sum -= window[-0x5] * b0[0x4];                     \
      sum -= window[-0x6] * b0[0x5];                     \
      sum -= window[-0x7] * b0[0x6];                     \
      sum -= window[-0x8] * b0[0x7];                     \
      sum -= window[-0x9] * b0[0x8];                     \
      sum -= window[-0xA] * b0[0x9];                     \
      sum -= window[-0xB] * b0[0xA];                     \
      sum -= window[-0xC] * b0[0xB];                     \
      sum -= window[-0xD] * b0[0xC];                     \
      sum -= window[-0xE] * b0[0xD];                     \
      sum -= window[-0xF] * b0[0xE];                     \
      sum -= window[-0x0] * b0[0xF];                     \
                                                         \
      WRITE_SAMPLE (TYPE,samples,sum,clip);              \
    }                                                    \
  }                                                      \
  *pnt += 64*sizeof(TYPE);                               \
                                                         \
  return clip;                                           
    /* *INDENT-ON* */


int
synth_1to1(PMPSTR mp, real * bandPtr, int channel, unsigned char *out, int *pnt)
{
    SYNTH_1TO1_CLIPCHOICE(short, WRITE_SAMPLE_CLIPPED)
} int
synth_1to1_unclipped(PMPSTR mp, real * bandPtr, int channel, unsigned char *out, int *pnt)
{
    SYNTH_1TO1_CLIPCHOICE(real, WRITE_SAMPLE_UNCLIPPED)
}
