/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: prototypes for extermal metrics specific to data type
 last mod: $Id: vqext.h 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#ifndef _V_VQEXT_
#define _V_VQEXT_

#include "vqgen.h"

extern char *vqext_booktype;
extern quant_meta q;
extern int vqext_aux;

extern float vqext_metric(vqgen *v,float *e, float *p);
extern float *vqext_weight(vqgen *v,float *p);
extern void vqext_addpoint_adj(vqgen *v,float *b,int start,int dim,int cols,int num);
extern void vqext_preprocess(vqgen *v);
extern void vqext_quantize(vqgen *v,quant_meta *);


#endif
