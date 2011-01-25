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

 function: utility for finding the distribution in a data set
 last mod: $Id: distribution.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "bookutil.h"

/* command line:
   distribution file.vqd
*/

int ascend(const void *a,const void *b){
  return(**((long **)a)-**((long **)b));
}

int main(int argc,char *argv[]){
  FILE *in;
  long lines=0;
  float min;
  float max;
  long bins=-1;
  int flag=0;
  long *countarray;
  long total=0;
  char *line;

  if(argv[1]==NULL){
    fprintf(stderr,"Usage: distribution {data.vqd [bins]| book.vqh} \n\n");
    exit(1);
  }
  if(argv[2]!=NULL)
    bins=atoi(argv[2])-1;

  in=fopen(argv[1],"r");
  if(!in){
    fprintf(stderr,"Could not open input file %s\n",argv[1]);
    exit(1);
  }

  if(strrchr(argv[1],'.') && strcmp(strrchr(argv[1],'.'),".vqh")==0){
    /* load/decode a book */

    codebook *b=codebook_load(argv[1]);
    static_codebook *c=(static_codebook *)(b->c);
    float delta;
    int i;
    fclose(in);

    switch(c->maptype){
    case 0:
      printf("entropy codebook only; no mappings\n");
      exit(0);
      break;
    case 1:
      bins=_book_maptype1_quantvals(c);
      break;
    case 2:
      bins=c->entries*c->dim;
      break;
    }

    max=min=_float32_unpack(c->q_min);
    delta=_float32_unpack(c->q_delta);

    for(i=0;i<bins;i++){
      float val=c->quantlist[i]*delta+min;
      if(val>max)max=val;
    }

    printf("Minimum scalar value: %f\n",min);
    printf("Maximum scalar value: %f\n",max);

    switch(c->maptype){
    case 1:
      {
	/* lattice codebook.  dump it. */
	int j,k;
	long maxcount=0;
	long **sort=calloc(bins,sizeof(long *));
	long base=c->lengthlist[0];
	countarray=calloc(bins,sizeof(long));

	for(i=0;i<bins;i++)sort[i]=c->quantlist+i;
	qsort(sort,bins,sizeof(long *),ascend);

	for(i=0;i<b->entries;i++)
	  if(c->lengthlist[i]>base)base=c->lengthlist[i];

	/* dump a full, correlated count */
	for(j=0;j<b->entries;j++){
	  if(c->lengthlist[j]){
	    int indexdiv=1;
	    printf("%4d: ",j);
	    for(k=0;k<b->dim;k++){	
	      int index= (j/indexdiv)%bins;
	      printf("%+3.1f,", c->quantlist[index]*_float32_unpack(c->q_delta)+
		     _float32_unpack(c->q_min));
	      indexdiv*=bins;
	    }
	    printf("\t|");
	    for(k=0;k<base-c->lengthlist[j];k++)printf("*");
	    printf("\n");
	  }
	}

	/* do a rough count */
	for(j=0;j<b->entries;j++){
	  int indexdiv=1;
	  for(k=0;k<b->dim;k++){
	    if(c->lengthlist[j]){
	      int index= (j/indexdiv)%bins;
	      countarray[index]+=(1<<(base-c->lengthlist[j]));
	      indexdiv*=bins;
	    }
	  }
	}

	/* dump the count */

	{
	  long maxcount=0,i,j;
	  for(i=0;i<bins;i++)
	    if(countarray[i]>maxcount)maxcount=countarray[i];
      
	  for(i=0;i<bins;i++){
	    int ptr=sort[i]-c->quantlist;
	    int stars=rint(50./maxcount*countarray[ptr]);
	    printf("%+08f (%8ld) |",c->quantlist[ptr]*delta+min,countarray[ptr]);
	    for(j=0;j<stars;j++)printf("*");
	    printf("\n");
	  }
	}
      }
      break;
    case 2:
      {
	/* trained, full mapping codebook. */
	printf("Can't do probability dump of a trained [type 2] codebook (yet)\n");
      }
      break;
    }
  }else{
    /* load/count a data file */

    /* do it the simple way; two pass. */
    line=setup_line(in);
    while(line){      
      float code;
      char buf[80];
      lines++;

      sprintf(buf,"getting min/max (%.2f::%.2f). lines...",min,max);
      if(!(lines&0xff))spinnit(buf,lines);
      
      while(!flag && sscanf(line,"%f",&code)==1){
	line=strchr(line,',');
	min=max=code;
	flag=1;
      }
      
      while(line && sscanf(line,"%f",&code)==1){
	line=strchr(line,',');
	if(line)line++;
	if(code<min)min=code;
	if(code>max)max=code;
      }
      
      line=setup_line(in);
    }
    
    if(bins<1){
      if((int)(max-min)==min-max){
	bins=max-min;
      }else{
	bins=25;
      }
    }
    
    printf("\r                                                     \r");
    printf("Minimum scalar value: %f\n",min);
    printf("Maximum scalar value: %f\n",max);

    if(argv[2]){
      
      printf("\n counting hits into %ld bins...\n",bins+1);
      countarray=calloc(bins+1,sizeof(long));
      
      rewind(in);
      line=setup_line(in);
      while(line){      
	float code;
	lines--;
	if(!(lines&0xff))spinnit("counting distribution. lines so far...",lines);
	
	while(line && sscanf(line,"%f",&code)==1){
	  line=strchr(line,',');
	  if(line)line++;
	  
	  code-=min;
	  code/=(max-min);
	  code*=bins;
	  countarray[(int)rint(code)]++;
	  total++;
	}
	
	line=setup_line(in);
      }
    
      /* make a pretty graph */
      {
	long maxcount=0,i,j;
	for(i=0;i<bins+1;i++)
	  if(countarray[i]>maxcount)maxcount=countarray[i];
	
	printf("\r                                                     \r");
	printf("Total scalars: %ld\n",total);
	for(i=0;i<bins+1;i++){
	  int stars=rint(50./maxcount*countarray[i]);
	  printf("%08f (%8ld) |",(max-min)/bins*i+min,countarray[i]);
	  for(j=0;j<stars;j++)printf("*");
	  printf("\n");
	}
      }
    }

    fclose(in);

  }
  printf("\nDone.\n");
  exit(0);
}
