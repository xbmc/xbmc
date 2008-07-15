/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: fft transform
 last mod: $Id: smallft.h,v 1.12 2002/07/11 06:40:50 xiphmont Exp $

 ********************************************************************/

#ifndef _V_SMFT_H_
#define _V_SMFT_H_

#include "vorbis/codec.h"

typedef struct {
  int n;
  float *trigcache;
  int *splitcache;
} drft_lookup;

extern void drft_forward(drft_lookup *l,float *data);
extern void drft_backward(drft_lookup *l,float *data);
extern void drft_init(drft_lookup *l,int n);
extern void drft_clear(drft_lookup *l);

#endif
