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

 function: generic euclidian distance metric for VQ codebooks
 last mod: $Id: genericdata.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="GENERICdata";  
int vqext_aux=0;                
quant_meta q={0,0,0,0};          /* non sequence data; each scalar 
				    independent */

void vqext_quantize(vqgen *v,quant_meta *q){
  vqgen_quantize(v,q);
}

float *vqext_weight(vqgen *v,float *p){
  /*noop*/
  return(p);
}

                            /* candidate,actual */
float vqext_metric(vqgen *v,float *e, float *p){
  int i;
  float acc=0.f;
  for(i=0;i<v->elements;i++){
    float val=p[i]-e[i];
    acc+=val*val;
  }
  return sqrt(acc/v->elements);
}

void vqext_addpoint_adj(vqgen *v,float *b,int start,int dim,int cols,int num){
  vqgen_addpoint(v,b+start,NULL);
}

void vqext_preprocess(vqgen *v){
  /* noop */
}






