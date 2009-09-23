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

 function: utility main for training codebooks
 last mod: $Id: train.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "vqgen.h"
#include "vqext.h"
#include "bookutil.h"

static char *rline(FILE *in,FILE *out,int pass){
  while(1){
    char *line=get_line(in);
    if(line && line[0]=='#'){
      if(pass)fprintf(out,"%s\n",line);
    }else{
      return(line);
    }
  }
}

/* command line:
   trainvq  vqfile [options] trainfile [trainfile]

   options: -params     entries,dim,quant
            -subvector  start[,num]
	    -error      desired_error
	    -iterations iterations
*/

static void usage(void){
  fprintf(stderr, "\nOggVorbis %s VQ codebook trainer\n\n"
	  "<foo>vqtrain vqfile [options] [datasetfile] [datasetfile]\n"
	  "options: -p[arams]     <entries,dim,quant>\n"
	  "         -s[ubvector]  <start[,num]>\n"
	  "         -e[rror]      <desired_error>\n"
	  "         -i[terations] <maxiterations>\n"
	  "         -d[istance]   quantization mesh spacing for density limitation\n"
	  "         -b <dummy>    eliminate cell size biasing; use normal LBG\n\n"
	  "         -c <dummy>    Use centroid (not median) midpoints\n"

	  "examples:\n"
	  "   train a new codebook to 1%% tolerance on datafile 'foo':\n"
	  "      xxxvqtrain book -p 256,6,8 -e .01 foo\n"
	  "      (produces a trained set in book-0.vqi)\n\n"
	  "   continue training 'book-0.vqi' (produces book-1.vqi):\n"
	  "      xxxvqtrain book-0.vqi\n\n"
	  "   add subvector from element 1 to <dimension> from files\n"
	  "      data*.m to the training in progress, prodicing book-1.vqi:\n"
	  "      xxxvqtrain book-0.vqi -s 1,1 data*.m\n\n",vqext_booktype);
}

int exiting=0;
void setexit(int dummy){
  fprintf(stderr,"\nexiting... please wait to finish this iteration\n");
  exiting=1;
}

int main(int argc,char *argv[]){
  vqgen v;

  int entries=-1,dim=-1;
  int start=0,num=-1;
  float desired=.05f,mindist=0.f;
  int iter=1000;
  int biasp=1;
  int centroid=0;

  FILE *out=NULL;
  char *line;
  long i,j,k;
  int init=0;
  q.quant=-1;

  argv++;
  if(!*argv){
    usage();
    exit(0);
  }

  /* get the book name, a preexisting book to continue training */
  {
    FILE *in=NULL;
    char *filename=alloca(strlen(*argv)+30),*ptr;

    strcpy(filename,*argv);
    in=fopen(filename,"r");
    ptr=strrchr(filename,'-');
    if(ptr){
      int num;
      ptr++;
      num=atoi(ptr);
      sprintf(ptr,"%d.vqi",num+1);
    }else
      strcat(filename,"-0.vqi");
    
    out=fopen(filename,"w");
    if(out==NULL){
      fprintf(stderr,"Unable to open %s for writing\n",filename);
      exit(1);
    }
    
    if(in){
      /* we wish to suck in a preexisting book and continue to train it */
      float a;
      
      line=rline(in,out,1);
      if(strcmp(line,vqext_booktype)){
	fprintf(stderr,"wrong book type; %s!=%s\n",line,vqext_booktype);
	exit(1);
      } 
      
      line=rline(in,out,1);
      if(sscanf(line,"%d %d %d",&entries,&dim,&vqext_aux)!=3){
	fprintf(stderr,"Syntax error reading book file\n");
	exit(1);
      }
      
      vqgen_init(&v,dim,vqext_aux,entries,mindist,
		 vqext_metric,vqext_weight,centroid);
      init=1;
      
      /* quant setup */
      line=rline(in,out,1);
      if(sscanf(line,"%ld %ld %d %d",&q.min,&q.delta,
		&q.quant,&q.sequencep)!=4){
	fprintf(stderr,"Syntax error reading book file\n");
	exit(1);
      }
      
      /* quantized entries */
      i=0;
      for(j=0;j<entries;j++){
	for(k=0;k<dim;k++){
	  line=rline(in,out,0);
	  sscanf(line,"%f",&a);
	  v.entrylist[i++]=a;
	}
      }      
      vqgen_unquantize(&v,&q);

      /* bias */
      i=0;
      for(j=0;j<entries;j++){
	line=rline(in,out,0);
	sscanf(line,"%f",&a);
	v.bias[i++]=a;
      }
      
      v.seeded=1;
      {
	float *b=alloca((dim+vqext_aux)*sizeof(float));
	i=0;
	while(1){
	  for(k=0;k<dim+vqext_aux;k++){
	    line=rline(in,out,0);
	    if(!line)break;
	    sscanf(line,"%f",b+k);
	  }
	  if(feof(in))break;
	  vqgen_addpoint(&v,b,b+dim);
	}
      }
      
      fclose(in);
    }
  }
  
  /* get the rest... */
  argv=argv++;
  while(*argv){
    if(argv[0][0]=='-'){
      /* it's an option */
      if(!argv[1]){
	fprintf(stderr,"Option %s missing argument.\n",argv[0]);
	exit(1);
      }
      switch(argv[0][1]){
      case 'p':
	if(sscanf(argv[1],"%d,%d,%d",&entries,&dim,&q.quant)!=3)
	  goto syner;
	break;
      case 's':
	if(sscanf(argv[1],"%d,%d",&start,&num)!=2){
	  num= -1;
	  if(sscanf(argv[1],"%d",&start)!=1)
	    goto syner;
	}
	break;
      case 'e':
	if(sscanf(argv[1],"%f",&desired)!=1)
	  goto syner;
	break;
      case 'd':
	if(sscanf(argv[1],"%f",&mindist)!=1)
	  goto syner;
	if(init)v.mindist=mindist;
	break;
      case 'i':
	if(sscanf(argv[1],"%d",&iter)!=1)
	  goto syner;
	break;
      case 'b':
	biasp=0;
	break;
      case 'c':
	centroid=1;
	break;
      default:
	fprintf(stderr,"Unknown option %s\n",argv[0]);
	exit(1);
      }
      argv+=2;
    }else{
      /* it's an input file */
      char *file=strdup(*argv++);
      FILE *in;
      int cols=-1;

      if(!init){
	if(dim==-1 || entries==-1 || q.quant==-1){
	  fprintf(stderr,"-p required when training a new set\n");
	  exit(1);
	}
	vqgen_init(&v,dim,vqext_aux,entries,mindist,
		   vqext_metric,vqext_weight,centroid);
	init=1;
      }

      in=fopen(file,"r");
      if(in==NULL){
	fprintf(stderr,"Could not open input file %s\n",file);
	exit(1);
      }
      fprintf(out,"# training file entry: %s\n",file);

      while((line=rline(in,out,0))){
	if(cols==-1){
	  char *temp=line;
	  while(*temp==' ')temp++;
	  for(cols=0;*temp;cols++){
	    while(*temp>32)temp++;
	    while(*temp==' ')temp++;
	  }

	  fprintf(stderr,"%d colums per line in file %s\n",cols,file);

	}
	{
	  int i;
	  float b[cols];
	  if(start+num*dim>cols){
	    fprintf(stderr,"ran out of columns reading %s\n",file);
	    exit(1);
	  }
	  while(*line==' ')line++;
	  for(i=0;i<cols;i++){

	    /* static length buffer bug workaround */
	    char *temp=line;
	    char old;
	    while(*temp>32)temp++;

	    old=temp[0];
	    temp[0]='\0';
	    b[i]=atof(line);
	    temp[0]=old;
	    
	    while(*line>32)line++;
	    while(*line==' ')line++;
	  }
	  if(num<=0)num=(cols-start)/dim;
	  for(i=0;i<num;i++)
	    vqext_addpoint_adj(&v,b,start+i*dim,dim,cols,num);

	}
      }
      fclose(in);
    }
  }

  if(!init){
    fprintf(stderr,"No input files!\n");
    exit(1);
  }

  vqext_preprocess(&v);

  /* train the book */
  signal(SIGTERM,setexit);
  signal(SIGINT,setexit);

  for(i=0;i<iter && !exiting;i++){
    float result;
    if(i!=0){
      vqgen_unquantize(&v,&q);
      vqgen_cellmetric(&v);
    }
    result=vqgen_iterate(&v,biasp);
    vqext_quantize(&v,&q);
    if(result<desired)break;
  }

  /* save the book */

  fprintf(out,"# OggVorbis VQ codebook trainer, intermediate file\n");
  fprintf(out,"%s\n",vqext_booktype);
  fprintf(out,"%d %d %d\n",entries,dim,vqext_aux);
  fprintf(out,"%ld %ld %d %d\n",
	  q.min,q.delta,q.quant,q.sequencep);

  /* quantized entries */
  fprintf(out,"# quantized entries---\n");
  i=0;
  for(j=0;j<entries;j++)
    for(k=0;k<dim;k++)
      fprintf(out,"%d\n",(int)(rint(v.entrylist[i++])));
  
  fprintf(out,"# biases---\n");
  i=0;
  for(j=0;j<entries;j++)
    fprintf(out,"%f\n",v.bias[i++]);

  /* we may have done the density limiting mesh trick; refetch the
     training points from the temp file */

  rewind(v.asciipoints);
  fprintf(out,"# points---\n");
  {
    /* sloppy, no error handling */
    long bytes;
    char buff[4096];
    while((bytes=fread(buff,1,4096,v.asciipoints)))
      while(bytes)bytes-=fwrite(buff,1,bytes,out);
  }

  fclose(out);
  fclose(v.asciipoints);

  vqgen_unquantize(&v,&q);
  vqgen_cellmetric(&v);
  exit(0);

  syner:
    fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
    exit(1);
}
