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

 function: build a VQ codebook 
 last mod: $Id: vqgen.h 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#ifndef _VQGEN_H_
#define _VQGEN_H_

typedef struct vqgen{
  int seeded;
  int sorted;

  int it;
  int elements;

  int aux;
  float mindist;
  int centroid;

  /* point cache */
  float *pointlist; 
  long   points;
  long   allocated;

  /* entries */
  float *entrylist;
  long   *assigned;
  float *bias;
  long   entries;
  float *max;
  
  float  (*metric_func) (struct vqgen *v,float *entry,float *point);
  float *(*weight_func) (struct vqgen *v,float *point);

  FILE *asciipoints;
} vqgen;

typedef struct {
  long   min;       /* packed 24 bit float */       
  long   delta;     /* packed 24 bit float */       
  int    quant;     /* 0 < quant <= 16 */
  int    sequencep; /* bitflag */
} quant_meta;

static inline float *_point(vqgen *v,long ptr){
  return v->pointlist+((v->elements+v->aux)*ptr);
}

static inline float *_aux(vqgen *v,long ptr){
  return _point(v,ptr)+v->aux;
}

static inline float *_now(vqgen *v,long ptr){
  return v->entrylist+(v->elements*ptr);
}

extern void vqgen_init(vqgen *v,
		       int elements,int aux,int entries,float mindist,
		       float  (*metric)(vqgen *,float *, float *),
		       float *(*weight)(vqgen *,float *),int centroid);
extern void vqgen_addpoint(vqgen *v, float *p,float *aux);

extern float vqgen_iterate(vqgen *v,int biasp);
extern void vqgen_unquantize(vqgen *v,quant_meta *q);
extern void vqgen_quantize(vqgen *v,quant_meta *q);
extern void vqgen_cellmetric(vqgen *v);

#endif





