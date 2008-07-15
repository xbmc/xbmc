/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: utility main for building thresh/pigeonhole encode hints
 last mod: $Id: latticehint.c,v 1.12 2001/12/20 01:00:39 segher Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "../lib/scales.h"
#include "bookutil.h"
#include "vqgen.h"
#include "vqsplit.h"

/* The purpose of this util is to build encode hints for lattice
   codebooks so that brute forcing each codebook entry isn't needed.
   Threshhold hints are for books in which each scalar in the vector
   is independant (eg, residue) and pigeonhole lookups provide a
   minimum error fit for words where the scalars are interdependant
   (each affecting the fit of the next in sequence) as in an LSP
   sequential book (or can be used along with a sparse threshhold map,
   like a splitting tree that need not be trained) 

   If the input book is non-sequential, a threshhold hint is built.
   If the input book is sequential, a pigeonholing hist is built.
   If the book is sparse, a pigeonholing hint is built, possibly in addition
     to the threshhold hint 

   command line:
   latticehint book.vqh [threshlist]

   latticehint produces book.vqh on stdout */

static int longsort(const void *a, const void *b){
  return(**((long **)a)-**((long **)b));
}

static int addtosearch(int entry,long **tempstack,long *tempcount,int add){
  long *ptr=tempstack[entry];
  long i=tempcount[entry];

  if(ptr){
    while(i--)
      if(*ptr++==add)return(0);
    tempstack[entry]=_ogg_realloc(tempstack[entry],
			     (tempcount[entry]+1)*sizeof(long));
  }else{
    tempstack[entry]=_ogg_malloc(sizeof(long));
  }

  tempstack[entry][tempcount[entry]++]=add;
  return(1);
}

static void setvals(int dim,encode_aux_pigeonhole *p,
		    long *temptrack,float *tempmin,float *tempmax,
		    int seqp){
  int i;
  float last=0.f;
  for(i=0;i<dim;i++){
    tempmin[i]=(temptrack[i])*p->del+p->min+last;
    tempmax[i]=tempmin[i]+p->del;
    if(seqp)last=tempmin[i];
  }
}

/* note that things are currently set up such that input fits that
   quantize outside the pigeonmap are dropped and brute-forced.  So we
   can ignore the <0 and >=n boundary cases in min/max error */

static float minerror(int dim,float *a,encode_aux_pigeonhole *p,
		       long *temptrack,float *tempmin,float *tempmax){
  int i;
  float err=0.f;
  for(i=0;i<dim;i++){
    float eval=0.f;
    if(a[i]<tempmin[i]){
      eval=tempmin[i]-a[i];
    }else if(a[i]>tempmax[i]){
      eval=a[i]-tempmax[i];
    }
    err+=eval*eval;
  }
  return(err);
}

static float maxerror(int dim,float *a,encode_aux_pigeonhole *p,
		       long *temptrack,float *tempmin,float *tempmax){
  int i;
  float err=0.f,eval;
  for(i=0;i<dim;i++){
    if(a[i]<tempmin[i]){
      eval=tempmax[i]-a[i];
    }else if(a[i]>tempmax[i]){
      eval=a[i]-tempmin[i];
    }else{
      float t1=a[i]-tempmin[i];
      eval=tempmax[i]-a[i];
      if(t1>eval)eval=t1;
    }
    err+=eval*eval;
  }
  return(err);
}

int main(int argc,char *argv[]){
  codebook *b;
  static_codebook *c;
  int entries=-1,dim=-1;
  float min,del;
  char *name;
  long i,j;
  float *suggestions;
  int suggcount=0;

  if(argv[1]==NULL){
    fprintf(stderr,"Need a lattice book on the command line.\n");
    exit(1);
  }

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
  min=_float32_unpack(c->q_min);
  del=_float32_unpack(c->q_delta);

  /* Do we want to gen a threshold hint? */
  if(c->q_sequencep==0){
    /* yes. Discard any preexisting threshhold hint */
    long quantvals=_book_maptype1_quantvals(c);
    long **quantsort=alloca(quantvals*sizeof(long *));
    encode_aux_threshmatch *t=_ogg_calloc(1,sizeof(encode_aux_threshmatch));
    c->thresh_tree=t;

    fprintf(stderr,"Adding threshold hint to %s...\n",name);

    /* partial/complete suggestions */
    if(argv[2]){
      char *ptr=strdup(argv[2]);
      suggestions=alloca(sizeof(float)*quantvals);
			 
      for(suggcount=0;ptr && suggcount<quantvals;suggcount++){
	char *ptr2=strchr(ptr,',');
	if(ptr2)*ptr2++='\0';
	suggestions[suggcount]=atof(ptr);
	ptr=ptr2;
      }
    }

    /* simplest possible threshold hint only */
    t->quantthresh=_ogg_calloc(quantvals-1,sizeof(float));
    t->quantmap=_ogg_calloc(quantvals,sizeof(int));
    t->threshvals=quantvals;
    t->quantvals=quantvals;

    /* the quantvals may not be in order; sort em first */
    for(i=0;i<quantvals;i++)quantsort[i]=c->quantlist+i;
    qsort(quantsort,quantvals,sizeof(long *),longsort);

    /* ok, gen the map and thresholds */
    for(i=0;i<quantvals;i++)t->quantmap[i]=quantsort[i]-c->quantlist;
    for(i=0;i<quantvals-1;i++){
      float v1=*(quantsort[i])*del+min;
      float v2=*(quantsort[i+1])*del+min;
      
      for(j=0;j<suggcount;j++)
	if(v1<suggestions[j] && suggestions[j]<v2){
	  t->quantthresh[i]=suggestions[j];
	  break;
	}
      
      if(j==suggcount){
	t->quantthresh[i]=(v1+v2)*.5;
      }
    }
  }

  /* Do we want to gen a pigeonhole hint? */
#if 0
  for(i=0;i<entries;i++)if(c->lengthlist[i]==0)break;
  if(c->q_sequencep || i<entries){
    long **tempstack;
    long *tempcount;
    long *temptrack;
    float *tempmin;
    float *tempmax;
    long totalstack=0;
    long pigeons;
    long subpigeons;
    long quantvals=_book_maptype1_quantvals(c);
    int changep=1,factor;

    encode_aux_pigeonhole *p=_ogg_calloc(1,sizeof(encode_aux_pigeonhole));
    c->pigeon_tree=p;

    fprintf(stderr,"Adding pigeonhole hint to %s...\n",name);
    
    /* the idea is that we quantize uniformly, even in a nonuniform
       lattice, so that quantization of one scalar has a predictable
       result on the next sequential scalar in a greedy matching
       algorithm.  We generate a lookup based on the quantization of
       the vector (pigeonmap groups quantized entries together) and
       list the entries that could possible be the best fit for any
       given member of that pigeonhole.  The encode process then has a
       much smaller list to brute force */

    /* find our pigeonhole-specific quantization values, fill in the
       quant value->pigeonhole map */
    factor=3;
    p->del=del;
    p->min=min;
    p->quantvals=quantvals;
    {
      int max=0;
      for(i=0;i<quantvals;i++)if(max<c->quantlist[i])max=c->quantlist[i];
      p->mapentries=max;
    }
    p->pigeonmap=_ogg_malloc(p->mapentries*sizeof(long));
    p->quantvals=(quantvals+factor-1)/factor;

    /* pigeonhole roughly on the boundaries of the quantvals; the
       exact pigeonhole grouping is an optimization issue, not a
       correctness issue */
    for(i=0;i<p->mapentries;i++){
      float thisval=del*i+min; /* middle of the quant zone */
      int quant=0;
      float err=fabs(c->quantlist[0]*del+min-thisval);
      for(j=1;j<quantvals;j++){
	float thiserr=fabs(c->quantlist[j]*del+min-thisval);
	if(thiserr<err){
	  quant=j/factor;
	  err=thiserr;
	}
      }
      p->pigeonmap[i]=quant;
    }
    
    /* pigeonmap complete.  Now do the grungy business of finding the
    entries that could possibly be the best fit for a value appearing
    in the pigeonhole. The trick that allows the below to work is the
    uniform quantization; even though the scalars may be 'sequential'
    (each a delta from the last), the uniform quantization means that
    the error variance is *not* dependant.  Given a pigeonhole and an
    entry, we can find the minimum and maximum possible errors
    (relative to the entry) for any point that could appear in the
    pigeonhole */
    
    /* must iterate over both pigeonholes and entries */
    /* temporarily (in order to avoid thinking hard), we grow each
       pigeonhole seperately, the build a stack of 'em later */
    pigeons=1;
    subpigeons=1;
    for(i=0;i<dim;i++)subpigeons*=p->mapentries;
    for(i=0;i<dim;i++)pigeons*=p->quantvals;
    temptrack=_ogg_calloc(dim,sizeof(long));
    tempmin=_ogg_calloc(dim,sizeof(float));
    tempmax=_ogg_calloc(dim,sizeof(float));
    tempstack=_ogg_calloc(pigeons,sizeof(long *));
    tempcount=_ogg_calloc(pigeons,sizeof(long));

    while(1){
      float errorpost=-1;
      char buffer[80];

      /* map our current pigeonhole to a 'big pigeonhole' so we know
         what list we're after */
      int entry=0;
      for(i=dim-1;i>=0;i--)entry=entry*p->quantvals+p->pigeonmap[temptrack[i]];
      setvals(dim,p,temptrack,tempmin,tempmax,c->q_sequencep);
      sprintf(buffer,"Building pigeonhole search list [%ld]...",totalstack);


      /* Search all entries to find the one with the minimum possible
         maximum error.  Record that error */
      for(i=0;i<entries;i++){
	if(c->lengthlist[i]>0){
	  float this=maxerror(dim,b->valuelist+i*dim,p,
			       temptrack,tempmin,tempmax);
	  if(errorpost==-1 || this<errorpost)errorpost=this;
	  spinnit(buffer,subpigeons);
	}
      }

      /* Our search list will contain all entries with a minimum
         possible error <= our errorpost */
      for(i=0;i<entries;i++)
	if(c->lengthlist[i]>0){
	  spinnit(buffer,subpigeons);
	  if(minerror(dim,b->valuelist+i*dim,p,
		      temptrack,tempmin,tempmax)<errorpost)
	    totalstack+=addtosearch(entry,tempstack,tempcount,i);
	}

      for(i=0;i<dim;i++){
	temptrack[i]++;
	if(temptrack[i]<p->mapentries)break;
	temptrack[i]=0;
      }
      if(i==dim)break;
      subpigeons--;
    }

    fprintf(stderr,"\r                                                     "
	    "\rTotal search list size (all entries): %ld\n",totalstack);

    /* pare the index of lists for improbable quantizations (where
       improbable is determined by c->lengthlist; we assume that
       pigeonholing is in sync with the codeword cells, which it is */
    /*for(i=0;i<entries;i++){
      float probability= 1.f/(1<<c->lengthlist[i]);
      if(c->lengthlist[i]==0 || probability*entries<cutoff){
	totalstack-=tempcount[i];
	tempcount[i]=0;
      }
      }*/

    /* pare the list of shortlists; merge contained and similar lists
       together */
    p->fitmap=_ogg_malloc(pigeons*sizeof(long));
    for(i=0;i<pigeons;i++)p->fitmap[i]=-1;
    while(changep){
      char buffer[80];
      changep=0;

      for(i=0;i<pigeons;i++){
	if(p->fitmap[i]<0 && tempcount[i]){
	  for(j=i+1;j<pigeons;j++){
	    if(p->fitmap[j]<0 && tempcount[j]){
	      /* is one list a superset, or are they sufficiently similar? */
	      int amiss=0,bmiss=0,ii,jj;
	      for(ii=0;ii<tempcount[i];ii++){
		for(jj=0;jj<tempcount[j];jj++)
		  if(tempstack[i][ii]==tempstack[j][jj])break;
		if(jj==tempcount[j])amiss++;
	      }
	      for(jj=0;jj<tempcount[j];jj++){
		for(ii=0;ii<tempcount[i];ii++)
		  if(tempstack[i][ii]==tempstack[j][jj])break;
		if(ii==tempcount[i])bmiss++;
	      }
	      if(amiss==0 ||
		 bmiss==0 ||
		 (amiss*2<tempcount[i] && bmiss*2<tempcount[j] &&
		 tempcount[i]+bmiss<entries/30)){

		/*superset/similar  Add all of one to the other. */
		for(jj=0;jj<tempcount[j];jj++)
		  totalstack+=addtosearch(i,tempstack,tempcount,
					  tempstack[j][jj]);
		totalstack-=tempcount[j];
		p->fitmap[j]=i;
		changep=1;
	      }
	    }
	  }
	  sprintf(buffer,"Consolidating [%ld total, %s]... ",totalstack,
		  changep?"reit":"nochange");
	  spinnit(buffer,pigeons-i);
	}
      }
    }

    /* repack the temp stack in final form */
    fprintf(stderr,"\r                                                       "
	    "\rFinal total list size: %ld\n",totalstack);
    

    p->fittotal=totalstack;
    p->fitlist=_ogg_malloc((totalstack+1)*sizeof(long));
    p->fitlength=_ogg_malloc(pigeons*sizeof(long));
    {
      long usage=0;
      for(i=0;i<pigeons;i++){
	if(p->fitmap[i]==-1){
	  if(tempcount[i])
	    memcpy(p->fitlist+usage,tempstack[i],tempcount[i]*sizeof(long));
	  p->fitmap[i]=usage;
	  p->fitlength[i]=tempcount[i];
	  usage+=tempcount[i];
	  if(usage>totalstack){
	    fprintf(stderr,"Internal error; usage>totalstack\n");
	    exit(1);
	  }
	}else{
	  p->fitlength[i]=p->fitlength[p->fitmap[i]];
	  p->fitmap[i]=p->fitmap[p->fitmap[i]];
	}
      }
    }
  }
#endif

  write_codebook(stdout,name,c); 
  fprintf(stderr,"\r                                                     "
	  "\nDone.\n");
  exit(0);
}
