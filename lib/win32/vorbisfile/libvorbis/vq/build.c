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

 function: utility main for building codebooks from training sets
 last mod: $Id: build.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "bookutil.h"

#include "vqgen.h"
#include "vqsplit.h"

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in,FILE *out){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar>=lbufsize){
	if(!lbufsize){	
	  lbufsize=1024;
	  linebuffer=_ogg_malloc(lbufsize);
	}else{
	  lbufsize*=2;
	  linebuffer=_ogg_realloc(linebuffer,lbufsize);
	}
      }
      {
	long c=fgetc(in);
	switch(c){
	case '\n':
	case EOF:
	  gotline=1;
	  break;
	default:
	  linebuffer[sofar++]=c;
	  linebuffer[sofar]='\0';
	  break;
	}
      }
    }
    
    if(linebuffer[0]=='#'){
      sofar=0;
    }else{
      return(linebuffer);
    }
  }
}

/* command line:
   buildvq file
*/

int main(int argc,char *argv[]){
  vqgen v;
  static_codebook c;
  codebook b;
  quant_meta q;

  long *quantlist=NULL;
  int entries=-1,dim=-1,aux=-1;
  FILE *out=NULL;
  FILE *in=NULL;
  char *line,*name;
  long i,j,k;

  b.c=&c;

  if(argv[1]==NULL){
    fprintf(stderr,"Need a trained data set on the command line.\n");
    exit(1);
  }

  {
    char *ptr;
    char *filename=strdup(argv[1]);

    in=fopen(filename,"r");
    if(!in){
      fprintf(stderr,"Could not open input file %s\n",filename);
      exit(1);
    }
    
    ptr=strrchr(filename,'-');
    if(ptr){
      *ptr='\0';
      name=strdup(filename);
      sprintf(ptr,".vqh");
    }else{
      name=strdup(filename);
      strcat(filename,".vqh");
    }

    out=fopen(filename,"w");
    if(out==NULL){
      fprintf(stderr,"Unable to open %s for writing\n",filename);
      exit(1);
    }
  }

  /* suck in the trained book */

  /* read book type, but it doesn't matter */
  line=rline(in,out);
  
  line=rline(in,out);
  if(sscanf(line,"%d %d %d",&entries,&dim,&aux)!=3){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* just use it to allocate mem */
  vqgen_init(&v,dim,0,entries,0.f,NULL,NULL,0);
  
  /* quant */
  line=rline(in,out);
  if(sscanf(line,"%ld %ld %d %d",&q.min,&q.delta,
	    &q.quant,&q.sequencep)!=4){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* quantized entries */
  /* save quant data; we don't want to requantize later as our method
     is currently imperfect wrt repeated application */
  i=0;
  quantlist=_ogg_malloc(sizeof(long)*v.elements*v.entries);
  for(j=0;j<entries;j++){
    float a;
    for(k=0;k<dim;k++){
      line=rline(in,out);
      sscanf(line,"%f",&a);
      v.entrylist[i]=a;
      quantlist[i++]=rint(a);
    }
  }    
  
  /* ignore bias */
  for(j=0;j<entries;j++)line=rline(in,out);
  free(v.bias);
  v.bias=NULL;
  
  /* training points */
  {
    float *b=alloca(sizeof(float)*(dim+aux));
    i=0;
    v.entries=0; /* hack to avoid reseeding */
    while(1){
      for(k=0;k<dim+aux;k++){
	line=rline(in,out);
	if(!line)break;
	sscanf(line,"%f",b+k);
      }
      if(feof(in))break;
      vqgen_addpoint(&v,b,NULL);
    }
    v.entries=entries;
  }
  
  fclose(in);
  vqgen_unquantize(&v,&q);

  /* build the book */
  vqsp_book(&v,&b,quantlist);
  c.q_min=q.min;
  c.q_delta=q.delta;
  c.q_quant=q.quant;
  c.q_sequencep=q.sequencep;

  /* save the book in C header form */
  write_codebook(out,name,b.c);

  fclose(out);
  exit(0);
}
