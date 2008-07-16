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

 function: residue backend 0 partitioner/classifier
 last mod: $Id: residuesplit.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "bookutil.h"

/* does not guard against invalid settings; eg, a subn of 16 and a
   subgroup request of 32.  Max subn of 128 */
static float _testhack(float *vec,int n){
  int i,j=0;
  float max=0.f;
  float temp[128];
  float entropy=0.;

  /* setup */
  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  /* handle case subgrp==1 outside */
  for(i=0;i<n;i++)
    if(temp[i]>max)max=temp[i];

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);

  for(i=0;i<n;i++)
    entropy+=temp[i];
  return entropy;

  /*while(1){
    entropy[j]=max;
    n>>=1;
    j++;

    if(n<=0)break;
    for(i=0;i<n;i++){
      temp[i]+=temp[i+n];
    }
    max=0.f;
    for(i=0;i<n;i++)
      if(temp[i]>max)max=temp[i];
      }*/
}

static FILE *of;
static FILE **or;

/* we evaluate the the entropy measure for each interleaved subgroup */
/* This is currently a bit specific to/hardwired for mapping 0; things
   will need to change in the future when we get real multichannel
   mappings */
int quantaux(float *res,int n,float *ebound,float *mbound,int *subgrp,int parts, int subn, 
	     int *class){
  long i,j,part=0;
  int aux;

  for(i=0;i<=n-subn;i+=subn,part++){
    float max=0.f;
    float lentropy=0.f;

    lentropy=_testhack(res+i,subn);

    for(j=0;j<subn;j++)
      if(fabs(res[i+j])>max)max=fabs(res[i+j]);

    for(j=0;j<parts-1;j++)
      if(lentropy<=ebound[j] &&
	 max<=mbound[j] &&
	 part<subgrp[j])
	break;
    class[part]=aux=j;
    
    fprintf(of,"%d, ",aux);
  }    
  fprintf(of,"\n");

  return(0);
}

int quantwrite(float *res,int n,int subn, int *class,int offset){
  long i,j,part=0;
  int aux;

  for(i=0;i<=n-subn;i+=subn,part++){
    aux=class[part];
    
    for(j=0;j<subn;j++)
      fprintf(or[aux+offset],"%g, ",res[j+i]);
    
    fprintf(or[aux+offset],"\n");
  }

  return(0);
}

static int getline(FILE *in,float *vec,int begin,int n){
  int i,next=0;

  reset_next_value();
  if(get_next_value(in,vec))return(0);
  if(begin){
    for(i=1;i<begin;i++)
      get_line_value(in,vec);
    next=0;
  }else{
    next=1;
  }

  for(i=next;i<n;i++)
    if(get_line_value(in,vec+i)){
      fprintf(stderr,"ran out of columns in input data\n");
      exit(1);
    }
  
  return(1);
}

static void usage(){
  fprintf(stderr,
	  "usage:\n" 
	  "residuesplit <res> [<res>] <begin,n,group> <baseout> <ent,peak,sub> [<ent,peak,sub>]...\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "         ent is the maximum entropy value allowed for membership in a group\n"
	  "         peak is the maximum amplitude value allowed for membership in a group\n"
	  "         subn is the maximum subpartiton number allowed in the group\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *buffer;
  char *base;
  int i,j,parts,begin,n,subn,*subgrp,*class;
  FILE **res;
  int resfiles=0;
  float *ebound,*mbound,*vec;
  long c=0;
  if(argc<5)usage();

  /* count the res file names, open the files */
  while(!strcmp(argv[resfiles+1]+strlen(argv[resfiles+1])-4,".vqd"))
    resfiles++;
  if(resfiles<1)usage();

  res=alloca(sizeof(*res)*resfiles);
  for(i=0;i<resfiles;i++){
    res[i]=fopen(argv[i+1],"r");
    if(!(res+i)){
      fprintf(stderr,"Could not open file %s\n",argv[1+i]);
      exit(1);
    }
  }

  base=strdup(argv[2+resfiles]);
  buffer=alloca(strlen(base)+20);
  {
    char *pos=strchr(argv[1+resfiles],',');
    begin=atoi(argv[1+resfiles]);
    if(!pos)
      usage();
    else
      n=atoi(pos+1);
    pos=strchr(pos+1,',');
    if(!pos)
      usage();
    else
      subn=atoi(pos+1);
    if(n/subn*subn != n){
      fprintf(stderr,"n must be divisible by group\n");
      exit(1);
    }
  }

  /* how many parts?... */
  parts=argc-resfiles-2;
  
  ebound=_ogg_malloc(sizeof(float)*parts);
  mbound=_ogg_malloc(sizeof(float)*parts);
  subgrp=_ogg_malloc(sizeof(int)*parts);
  
  for(i=0;i<parts-1;i++){
    char *pos=strchr(argv[3+i+resfiles],',');
    subgrp[i]=0;
    if(*argv[3+i+resfiles]==',')
      ebound[i]=1e50f;
    else
      ebound[i]=atof(argv[3+i+resfiles]);

    if(!pos){
      mbound[i]=1e50f;
    }else{
      if(*(pos+1)==',')
	mbound[i]=1e50f;
      else
	mbound[i]=atof(pos+1);
      pos=strchr(pos+1,',');
      
       if(pos)
	 subgrp[i]=atoi(pos+1);
       
    }
    if(subgrp[i]<=0)subgrp[i]=99999;
  }

  ebound[i]=1e50f;
  mbound[i]=1e50f;
  subgrp[i]=9999999;

  or=alloca(parts*resfiles*sizeof(FILE*));
  sprintf(buffer,"%saux.vqd",base);
  of=fopen(buffer,"w");
  if(!of){
    fprintf(stderr,"Could not open file %s for writing\n",buffer);
    exit(1);
  }

  for(j=0;j<resfiles;j++){
    for(i=0;i<parts;i++){
      sprintf(buffer,"%s_%d%c.vqd",base,i,j+65);
      or[i+j*parts]=fopen(buffer,"w");
      if(!or[i+j*parts]){
	fprintf(stderr,"Could not open file %s for writing\n",buffer);
	exit(1);
      }
    }
  }
  
  vec=_ogg_malloc(sizeof(float)*n);
  class=_ogg_malloc(sizeof(float)*n);
  /* get the input line by line and process it */
  while(1){
    if(getline(res[0],vec,begin,n)){
      quantaux(vec,n,ebound,mbound,subgrp,parts,subn,class);
      quantwrite(vec,n,subn,class,0);

      for(i=1;i<resfiles;i++){
	if(getline(res[i],vec,begin,n)){
	  quantwrite(vec,n,subn,class,parts*i);
	}else{
	  fprintf(stderr,"Getline loss of sync (%d).\n\n",i);
	  exit(1);
	}
      }
    }else{
      if(feof(res[0]))break;
      fprintf(stderr,"Getline loss of sync (0).\n\n");
      exit(1);
    }
    
    c++;
    if(!(c&0xf)){
      spinnit("kB so far...",(int)(ftell(res[0])/1024));
    }
  }
  for(i=0;i<resfiles;i++)
    fclose(res[i]);
  fclose(of);
  for(i=0;i<parts*resfiles;i++)
    fclose(or[i]);
  fprintf(stderr,"\rDone                         \n");
  return(0);
}




