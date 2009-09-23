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

 function: function calls to collect codebook metrics
 last mod: $Id: metrics.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "bookutil.h"

/* collect the following metrics:

   mean and mean squared amplitude
   mean and mean squared error 
   mean and mean squared error (per sample) by entry
   worst case fit by entry
   entry cell size
   hits by entry
   total bits
   total samples
   (average bits per sample)*/
   

/* set up metrics */

float meanamplitude_acc=0.f;
float meanamplitudesq_acc=0.f;
float meanerror_acc=0.f;
float meanerrorsq_acc=0.f;

float **histogram=NULL;
float **histogram_error=NULL;
float **histogram_errorsq=NULL;
float **histogram_hi=NULL;
float **histogram_lo=NULL;
float bits=0.f;
float count=0.f;

static float *_now(codebook *c, int i){
  return c->valuelist+i*c->c->dim;
}

int books=0;

void process_preprocess(codebook **bs,char *basename){
  int i;
  while(bs[books])books++;
  
  if(books){
    histogram=_ogg_calloc(books,sizeof(float *));
    histogram_error=_ogg_calloc(books,sizeof(float *));
    histogram_errorsq=_ogg_calloc(books,sizeof(float *));
    histogram_hi=_ogg_calloc(books,sizeof(float *));
    histogram_lo=_ogg_calloc(books,sizeof(float *));
  }else{
    fprintf(stderr,"Specify at least one codebook\n");
    exit(1);
  }

  for(i=0;i<books;i++){
    codebook *b=bs[i];
    histogram[i]=_ogg_calloc(b->entries,sizeof(float));
    histogram_error[i]=_ogg_calloc(b->entries*b->dim,sizeof(float));
    histogram_errorsq[i]=_ogg_calloc(b->entries*b->dim,sizeof(float));
    histogram_hi[i]=_ogg_calloc(b->entries*b->dim,sizeof(float));
    histogram_lo[i]=_ogg_calloc(b->entries*b->dim,sizeof(float));
  }
}

static float _dist(int el,float *a, float *b){
  int i;
  float acc=0.f;
  for(i=0;i<el;i++){
    float val=(a[i]-b[i]);
    acc+=val*val;
  }
  return acc;
}

void cell_spacing(codebook *c){
  int j,k;
  float min=-1.f,max=-1.f,mean=0.f,meansq=0.f;
  long total=0;

  /* minimum, maximum, mean, ms cell spacing */
  for(j=0;j<c->c->entries;j++){
    if(c->c->lengthlist[j]>0){
      float localmin=-1.;
      for(k=0;k<c->c->entries;k++){
	if(c->c->lengthlist[k]>0){
	  float this=_dist(c->c->dim,_now(c,j),_now(c,k));
	  if(j!=k &&
	     (localmin==-1 || this<localmin))
	    localmin=this;
	}
      }
      
      if(min==-1 || localmin<min)min=localmin;
      if(max==-1 || localmin>max)max=localmin;
      mean+=sqrt(localmin);
      meansq+=localmin;
      total++;
    }
  }
  
  fprintf(stderr,"\tminimum cell spacing (closest side): %g\n",sqrt(min));
  fprintf(stderr,"\tmaximum cell spacing (closest side): %g\n",sqrt(max));
  fprintf(stderr,"\tmean closest side spacing: %g\n",mean/total);
  fprintf(stderr,"\tmean sq closest side spacing: %g\n",sqrt(meansq/total));
}

void process_postprocess(codebook **bs,char *basename){
  int i,k,book;
  char *buffer=alloca(strlen(basename)+80);

  fprintf(stderr,"Done.  Processed %ld data points:\n\n",
	  (long)count);

  fprintf(stderr,"Global statistics:******************\n\n");

  fprintf(stderr,"\ttotal samples: %ld\n",(long)count);
  fprintf(stderr,"\ttotal bits required to code: %ld\n",(long)bits);
  fprintf(stderr,"\taverage bits per sample: %g\n\n",bits/count);

  fprintf(stderr,"\tmean sample amplitude: %g\n",
	  meanamplitude_acc/count);
  fprintf(stderr,"\tmean squared sample amplitude: %g\n\n",
	  sqrt(meanamplitudesq_acc/count));

  fprintf(stderr,"\tmean code error: %g\n",
	  meanerror_acc/count);
  fprintf(stderr,"\tmean squared code error: %g\n\n",
	  sqrt(meanerrorsq_acc/count));

  for(book=0;book<books;book++){
    FILE *out;
    codebook *b=bs[book];
    int n=b->c->entries;
    int dim=b->c->dim;

    fprintf(stderr,"Book %d statistics:------------------\n",book);

    cell_spacing(b);

    sprintf(buffer,"%s-%d-mse.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		sqrt((histogram_errorsq[book]+i*dim)[k]/histogram[book][i]));
      }
    }
    fclose(out);
      
    sprintf(buffer,"%s-%d-me.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		(histogram_error[book]+i*dim)[k]/histogram[book][i]);
      }
    }
    fclose(out);

    sprintf(buffer,"%s-%d-worst.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		(b->valuelist+i*dim)[k]+(histogram_lo[book]+i*dim)[k],
		(b->valuelist+i*dim)[k]+(histogram_hi[book]+i*dim)[k]);
      }
    }
    fclose(out);
  }
}

float process_one(codebook *b,int book,float *a,int dim,int step,int addmul,
		   float base){
  int j,entry;
  float amplitude=0.f;

  if(book==0){
    float last=base;
    for(j=0;j<dim;j++){
      amplitude=a[j*step]-(b->c->q_sequencep?last:0);
      meanamplitude_acc+=fabs(amplitude);
      meanamplitudesq_acc+=amplitude*amplitude;
      count++;
      last=a[j*step];
    }
  }

  if(b->c->q_sequencep){
    float temp;
    for(j=0;j<dim;j++){
      temp=a[j*step];
      a[j*step]-=base;
    }
    base=temp;
  }

  entry=vorbis_book_besterror(b,a,step,addmul);

  if(entry==-1){
    fprintf(stderr,"Internal error: _best returned -1.\n");
    exit(1);
  }
  
  histogram[book][entry]++;  
  bits+=vorbis_book_codelen(b,entry);
	  
  for(j=0;j<dim;j++){
    float error=a[j*step];

    if(book==books-1){
      meanerror_acc+=fabs(error);
      meanerrorsq_acc+=error*error;
    }
    histogram_errorsq[book][entry*dim+j]+=error*error;
    histogram_error[book][entry*dim+j]+=fabs(error);
    if(histogram[book][entry]==0 || histogram_hi[book][entry*dim+j]<error)
      histogram_hi[book][entry*dim+j]=error;
    if(histogram[book][entry]==0 || histogram_lo[book][entry*dim+j]>error)
      histogram_lo[book][entry*dim+j]=error;
  }
  return base;
}


void process_vector(codebook **bs,int *addmul,int inter,float *a,int n){
  int bi;
  int i;

  for(bi=0;bi<books;bi++){
    codebook *b=bs[bi];
    int dim=b->dim;
    float base=0.f;

    if(inter){
      for(i=0;i<n/dim;i++)
	base=process_one(b,bi,a+i,dim,n/dim,addmul[bi],base);
    }else{
      for(i=0;i<=n-dim;i+=dim)
	base=process_one(b,bi,a+i,dim,1,addmul[bi],base);
    }
  }
  
  if((long)(count)%100)spinnit("working.... samples: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqmetrics [-i] +|*<codebook>.vqh [ +|*<codebook.vqh> ]... \n"
	  "                 datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  -i indicates interleaved coding.\n"
	  "       Output goes to output files:\n"
	  "       basename-me.m:       gnuplot: mean error by entry value\n"
	  "       basename-mse.m:      gnuplot: mean square error by entry value\n"
	  "       basename-worst.m:    gnuplot: worst error by entry value\n"
	  "       basename-distance.m: gnuplot file showing distance probability\n"
	  "\n");

}
