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

 function: metrics and quantization code for LSP VQ codebooks
 last mod: $Id: lspdata.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"
#include "codebook.h"

char *vqext_booktype="LSPdata";  
quant_meta q={0,0,0,1};          /* set sequence data */
int vqext_aux=1;

float global_maxdel=M_PI;
float global_mindel=M_PI;
#if 0
void vqext_quantize(vqgen *v,quant_meta *q){
  float delta,mindel;
  float maxquant=((1<<q->quant)-1);
  int j,k;

  /* first find the basic delta amount from the maximum span to be
     encoded.  Loosen the delta slightly to allow for additional error
     during sequence quantization */

  delta=(global_maxdel-global_mindel)/((1<<q->quant)-1.5f);
  
  q->min=_float32_pack(global_mindel);
  q->delta=_float32_pack(delta);

  mindel=_float32_unpack(q->min);
  delta=_float32_unpack(q->delta);

  for(j=0;j<v->entries;j++){
    float last=0;
    for(k=0;k<v->elements;k++){
      float val=_now(v,j)[k];
      float now=rint((val-last-mindel)/delta);
      
      _now(v,j)[k]=now;
      if(now<0){
	/* be paranoid; this should be impossible */
	fprintf(stderr,"fault; quantized value<0\n");
	exit(1);
      }

      if(now>maxquant){
	/* be paranoid; this should be impossible */
	fprintf(stderr,"fault; quantized value>max\n");
	exit(1);
      }
      last=(now*delta)+mindel+last;
    }
  }

}
#else
void vqext_quantize(vqgen *v,quant_meta *q){
  vqgen_quantize(v,q);
}
#endif

float *weight=NULL;
#if 0
/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

#define FUDGE (global_maxdel-weight[i])

float *vqext_weight(vqgen *v,float *p){
  int i;
  int el=v->elements;
  float lastp=0.f;
  for(i=0;i<el;i++){
    float predist=(p[i]-lastp);
    float postdist=(p[i+1]-p[i]);
    weight[i]=(predist<postdist?predist:postdist);
    lastp=p[i];
  }
  return p;
}
#else
#define FUDGE 1.f
float *vqext_weight(vqgen *v,float *p){
  return p;
}
#endif

                            /* candidate,actual */
float vqext_metric(vqgen *v,float *e, float *p){
  int i;
  int el=v->elements;
  float acc=0.f;
  for(i=0;i<el;i++){
    float val=(p[i]-e[i])*FUDGE;
    acc+=val*val;
  }
  return sqrt(acc/v->elements);
}

/* Data files are line-vectors, now just deltas.  The codebook entries
   want to be monotonically increasing, so we adjust */

/* assume vqext_aux==1 */
void vqext_addpoint_adj(vqgen *v,float *b,int start,int dim,int cols,int num){
  float *a=alloca(sizeof(float)*(dim+1)); /* +aux */
  float base=0;
  int i;

  for(i=0;i<dim;i++)
    base=a[i]=b[i+start]+base;

  if(start+dim+1>cols) /* +aux */
    a[i]=M_PI;
  else
    a[i]=b[i+start]+base;
  
  vqgen_addpoint(v,a,a+dim);
}

/* we just need to calc the global_maxdel from the training set */
void vqext_preprocess(vqgen *v){
  long j,k;

  global_maxdel=0.f;
  global_mindel=M_PI;
  for(j=0;j<v->points;j++){
    float last=0.;
    for(k=0;k<v->elements+v->aux;k++){
      float p=_point(v,j)[k];
      if(p-last>global_maxdel)global_maxdel=p-last;
      if(p-last<global_mindel)global_mindel=p-last;
      last=p;
    }
  }

  weight=_ogg_malloc(sizeof(float)*v->elements);
}

