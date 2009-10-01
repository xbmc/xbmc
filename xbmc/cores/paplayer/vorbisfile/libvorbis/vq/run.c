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

 function: utility main for loading and operating on codebooks
 last mod: $Id: run.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "bookutil.h"

/* command line:
   utilname [-i] +|* input_book.vqh [+|* input_book.vqh] 
            input_data.vqd [input_data.vqd]

   produces output data on stdout
   (may also take input data from stdin)

 */

extern void process_preprocess(codebook **b,char *basename);
extern void process_postprocess(codebook **b,char *basename);
extern void process_vector(codebook **b,int *addmul, int inter,float *a,int n);
extern void process_usage(void);

int main(int argc,char *argv[]){
  char *basename;
  codebook **b=_ogg_calloc(1,sizeof(codebook *));
  int *addmul=_ogg_calloc(1,sizeof(int));
  int books=0;
  int input=0;
  int interleave=0;
  int j;
  int start=0;
  int num=-1;
  argv++;

  if(*argv==NULL){
    process_usage();
    exit(1);
  }

  /* yes, this is evil.  However, it's very convenient to parse file
     extentions */

  while(*argv){
    if(*argv[0]=='-'){
      /* option */
      if(argv[0][1]=='s'){
	/* subvector */
	if(sscanf(argv[1],"%d,%d",&start,&num)!=2){
	  num= -1;
	  if(sscanf(argv[1],"%d",&start)!=1){
	    fprintf(stderr,"Syntax error using -s\n");
	    exit(1);
	  }
	}
	argv+=2;
      }
      if(argv[0][1]=='i'){
	/* interleave */
	interleave=1;
	argv+=1;
      }
    }else{
      /* input file.  What kind? */
      char *dot;
      char *ext=NULL;
      char *name=strdup(*argv++);
      dot=strrchr(name,'.');
      if(dot)
	ext=dot+1;
      else
	ext="";

      /* codebook */
      if(!strcmp(ext,"vqh")){
	int multp=0;
	if(input){
	  fprintf(stderr,"specify all input data (.vqd) files following\n"
		  "codebook header (.vqh) files\n");
	  exit(1);
	}
	/* is it additive or multiplicative? */
	if(name[0]=='*'){
	  multp=1;
	  name++;
	}
	if(name[0]=='+')name++;

	basename=strrchr(name,'/');
	if(basename)
	  basename=strdup(basename)+1;
	else
	  basename=strdup(name);
	dot=strrchr(basename,'.');
	if(dot)*dot='\0';

	b=_ogg_realloc(b,sizeof(codebook *)*(books+2));
	b[books]=codebook_load(name);
	addmul=_ogg_realloc(addmul,sizeof(int)*(books+1));
	addmul[books++]=multp;
	b[books]=NULL;
      }

      /* data file */
      if(!strcmp(ext,"vqd")){
	int cols;
	long lines=0;
	char *line;
	float *vec;
	FILE *in=fopen(name,"r");
	if(!in){
	  fprintf(stderr,"Could not open input file %s\n",name);
	  exit(1);
	}

	if(!input){
	  process_preprocess(b,basename);
	  input++;
	}

	reset_next_value();
	line=setup_line(in);
	/* count cols before we start reading */
	{
	  char *temp=line;
	  while(*temp==' ')temp++;
	  for(cols=0;*temp;cols++){
	    while(*temp>32)temp++;
	    while(*temp==' ')temp++;
	  }
	}
	vec=alloca(cols*sizeof(float));
	while(line){
	  lines++;
	  for(j=0;j<cols;j++)
	    if(get_line_value(in,vec+j)){
	      fprintf(stderr,"Too few columns on line %ld in data file\n",lines);
	      exit(1);
	    }
	  /* ignores -s for now */
	  process_vector(b,addmul,interleave,vec,cols);

	  line=setup_line(in);
	}
	fclose(in);
      }
    }
  }

  /* take any data from stdin */
  {
    struct stat st;
    if(fstat(STDIN_FILENO,&st)==-1){
      fprintf(stderr,"Could not stat STDIN\n");
      exit(1);
    }
    if((S_IFIFO|S_IFREG|S_IFSOCK)&st.st_mode){
      int cols;
      char *line;
      long lines=0;
      float *vec;
      if(!input){
	process_preprocess(b,basename);
	input++;
      }
      
      line=setup_line(stdin);
      /* count cols before we start reading */
      {
	char *temp=line;
	while(*temp==' ')temp++;
	for(cols=0;*temp;cols++){
	  while(*temp>32)temp++;
	  while(*temp==' ')temp++;
	}
      }
      vec=alloca(cols*sizeof(float));
      while(line){
	lines++;
	for(j=0;j<cols;j++)
	  if(get_line_value(stdin,vec+j)){
	    fprintf(stderr,"Too few columns on line %ld in data file\n",lines);
	    exit(1);
	  }
	/* ignores -s for now */
	process_vector(b,addmul,interleave,vec,cols);
	
	line=setup_line(stdin);
      }
    }
  }

  process_postprocess(b,basename);

  return 0;
}
