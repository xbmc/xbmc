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

 function: function call to do simple data cascading
 last mod: $Id: cascade.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

/* this one outputs residue to stdout. */

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "bookutil.h"

/* set up metrics */

float count=0.f;


void process_preprocess(codebook **bs,char *basename){
}

void process_postprocess(codebook **b,char *basename){
  fprintf(stderr,"Done.                      \n");
}

float process_one(codebook *b,float *a,int dim,int step,int addmul,
		   float base){
  int j;

  if(b->c->q_sequencep){
    float temp;
    for(j=0;j<dim;j++){
      temp=a[j*step];
      a[j*step]-=base;
    }
    base=temp;
  }

  vorbis_book_besterror(b,a,step,addmul);
  
  return base;
}

void process_vector(codebook **bs,int *addmul,int inter,float *a,int n){
  int i,bi=0;
  int booknum=0;
  
  while(*bs){
    float base=0.f;
    codebook *b=*bs;
    int dim=b->dim;
    
    if(inter){
      for(i=0;i<n/dim;i++)
	base=process_one(b,a+i,dim,n/dim,addmul[bi],base);
    }else{
      for(i=0;i<=n-dim;i+=dim)
	base=process_one(b,a+i,dim,1,addmul[bi],base);
    }

    bs++;
    booknum++;
    bi++;
  }

  for(i=0;i<n;i++)
    fprintf(stdout,"%f, ",a[i]);
  fprintf(stdout,"\n");
  
  if((long)(count++)%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqcascade [-i] +|*<codebook>.vqh [ +|*<codebook.vqh> ]... \n"
	  "                 datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  residual error data sent to\n"
	  "       stdout.\n\n");

}
