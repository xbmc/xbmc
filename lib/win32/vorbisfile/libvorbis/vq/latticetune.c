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

 function: utility main for setting entropy encoding parameters
           for lattice codebooks
 last mod: $Id: latticetune.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "bookutil.h"

static int strrcmp_i(char *s,char *cmp){
  return(strncmp(s+strlen(s)-strlen(cmp),cmp,strlen(cmp)));
}

/* This util takes a training-collected file listing codewords used in
   LSP fitting, then generates new codeword lengths for maximally
   efficient integer-bits entropy encoding.

   command line:
   latticetune book.vqh input.vqd [unused_entriesp]

   latticetune produces book.vqh on stdout */

int main(int argc,char *argv[]){
  codebook *b;
  static_codebook *c;
  long *lengths;
  long *hits;

  int entries=-1,dim=-1,guard=1;
  FILE *in=NULL;
  char *line,*name;
  long j;

  if(argv[1]==NULL){
    fprintf(stderr,"Need a lattice codebook on the command line.\n");
    exit(1);
  }
  if(argv[2]==NULL){
    fprintf(stderr,"Need a codeword data file on the command line.\n");
    exit(1);
  }
  if(argv[3]!=NULL)guard=0;

  {
    char *ptr;
    char *filename=strdup(argv[1]);

    b=codebook_load(filename);
    c=(static_codebook *)(b->c);
    
    ptr=strrchr(filename,'.');
    if(ptr){
      *ptr='\0';
      name=strdup(filename);
    }else{
      name=strdup(filename);
    }
  }

  if(c->maptype!=1){
    fprintf(stderr,"Provided book is not a latticebook.\n");
    exit(1);
  }

  entries=b->entries;
  dim=b->dim;

  hits=_ogg_malloc(entries*sizeof(long));
  lengths=_ogg_calloc(entries,sizeof(long));
  for(j=0;j<entries;j++)hits[j]=guard;

  in=fopen(argv[2],"r");
  if(!in){
    fprintf(stderr,"Could not open input file %s\n",argv[2]);
    exit(1);
  }

  if(!strrcmp_i(argv[0],"latticetune")){
    long lines=0;
    line=setup_line(in);
    while(line){      
      long code;
      lines++;
      if(!(lines&0xfff))spinnit("codewords so far...",lines);
      
      if(sscanf(line,"%ld",&code)==1)
	hits[code]++;

      line=setup_line(in);
    }
  }

  /* now we simply count already collated by-entry data */
  if(!strrcmp_i(argv[0],"res0tune") || !strrcmp_i(argv[0],"res1tune")){

    line=setup_line(in);
    while(line){

      /* code:hits\n */
      /* likely to have multiple listing for each code entry; must
         accumulate */

      char *pos=strchr(line,':');
      if(pos){
	long code=atol(line);
	long val=atol(pos+1); 
	hits[code]+=val;
      }

      line=setup_line(in);
    }
  }

  fclose(in);

  /* build the codeword lengths */
  build_tree_from_lengths0(entries,hits,lengths);

  c->lengthlist=lengths;
  write_codebook(stdout,name,c); 

  {
    long bins=_book_maptype1_quantvals(c);
    long i,k,base=c->lengthlist[0];
    for(i=0;i<entries;i++)
      if(c->lengthlist[i]>base)base=c->lengthlist[i];
    
    for(j=0;j<entries;j++){
      if(c->lengthlist[j]){
	int indexdiv=1;
	fprintf(stderr,"%4ld: ",j);
	for(k=0;k<c->dim;k++){      
	  int index= (j/indexdiv)%bins;
	  fprintf(stderr,"%+3.1f,", c->quantlist[index]*_float32_unpack(c->q_delta)+
		 _float32_unpack(c->q_min));
	  indexdiv*=bins;
	}
	fprintf(stderr,"\t|");
	for(k=0;k<base-c->lengthlist[j];k++)fprintf(stderr,"*");
	fprintf(stderr,"\n");
      }
    }
  }
  
  fprintf(stderr,"\r                                                     "
	  "\nDone.\n");
  exit(0);
}
