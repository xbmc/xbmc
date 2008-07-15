/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: basic codebook pack/unpack/code/decode operations
 last mod: $Id: codebook.c,v 1.39 2002/06/28 22:19:35 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codebook.h"
#include "scales.h"
#include "misc.h"
#include "os.h"

/* packs the given codebook into the bitstream **************************/

int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *opb){
  long i,j;
  int ordered=0;

  /* first the basic parameters */
  oggpack_write(opb,0x564342,24);
  oggpack_write(opb,c->dim,16);
  oggpack_write(opb,c->entries,24);

  /* pack the codewords.  There are two packings; length ordered and
     length random.  Decide between the two now. */
  
  for(i=1;i<c->entries;i++)
    if(c->lengthlist[i-1]==0 || c->lengthlist[i]<c->lengthlist[i-1])break;
  if(i==c->entries)ordered=1;
  
  if(ordered){
    /* length ordered.  We only need to say how many codewords of
       each length.  The actual codewords are generated
       deterministically */

    long count=0;
    oggpack_write(opb,1,1);  /* ordered */
    oggpack_write(opb,c->lengthlist[0]-1,5); /* 1 to 32 */

    for(i=1;i<c->entries;i++){
      long this=c->lengthlist[i];
      long last=c->lengthlist[i-1];
      if(this>last){
	for(j=last;j<this;j++){
	  oggpack_write(opb,i-count,_ilog(c->entries-count));
	  count=i;
	}
      }
    }
    oggpack_write(opb,i-count,_ilog(c->entries-count));
    
  }else{
    /* length random.  Again, we don't code the codeword itself, just
       the length.  This time, though, we have to encode each length */
    oggpack_write(opb,0,1);   /* unordered */
    
    /* algortihmic mapping has use for 'unused entries', which we tag
       here.  The algorithmic mapping happens as usual, but the unused
       entry has no codeword. */
    for(i=0;i<c->entries;i++)
      if(c->lengthlist[i]==0)break;

    if(i==c->entries){
      oggpack_write(opb,0,1); /* no unused entries */
      for(i=0;i<c->entries;i++)
	oggpack_write(opb,c->lengthlist[i]-1,5);
    }else{
      oggpack_write(opb,1,1); /* we have unused entries; thus we tag */
      for(i=0;i<c->entries;i++){
	if(c->lengthlist[i]==0){
	  oggpack_write(opb,0,1);
	}else{
	  oggpack_write(opb,1,1);
	  oggpack_write(opb,c->lengthlist[i]-1,5);
	}
      }
    }
  }

  /* is the entry number the desired return value, or do we have a
     mapping? If we have a mapping, what type? */
  oggpack_write(opb,c->maptype,4);
  switch(c->maptype){
  case 0:
    /* no mapping */
    break;
  case 1:case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */
    
    if(!c->quantlist){
      /* no quantlist?  error */
      return(-1);
    }
    
    /* values that define the dequantization */
    oggpack_write(opb,c->q_min,32);
    oggpack_write(opb,c->q_delta,32);
    oggpack_write(opb,c->q_quant-1,4);
    oggpack_write(opb,c->q_sequencep,1);
    
    {
      int quantvals;
      switch(c->maptype){
      case 1:
	/* a single column of (c->entries/c->dim) quantized values for
	   building a full value list algorithmically (square lattice) */
	quantvals=_book_maptype1_quantvals(c);
	break;
      case 2:
	/* every value (c->entries*c->dim total) specified explicitly */
	quantvals=c->entries*c->dim;
	break;
      default: /* NOT_REACHABLE */
	quantvals=-1;
      }

      /* quantized values */
      for(i=0;i<quantvals;i++)
	oggpack_write(opb,labs(c->quantlist[i]),c->q_quant);

    }
    break;
  default:
    /* error case; we don't have any other map types now */
    return(-1);
  }

  return(0);
}

/* unpacks a codebook from the packet buffer into the codebook struct,
   readies the codebook auxiliary structures for decode *************/
int vorbis_staticbook_unpack(oggpack_buffer *opb,static_codebook *s){
  long i,j;
  memset(s,0,sizeof(*s));
  s->allocedp=1;

  /* make sure alignment is correct */
  if(oggpack_read(opb,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  s->dim=oggpack_read(opb,16);
  s->entries=oggpack_read(opb,24);
  if(s->entries==-1)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch((int)oggpack_read(opb,1)){
  case 0:
    /* unordered */
    s->lengthlist=_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

    /* allocated but unused entries? */
    if(oggpack_read(opb,1)){
      /* yes, unused entries */

      for(i=0;i<s->entries;i++){
	if(oggpack_read(opb,1)){
	  long num=oggpack_read(opb,5);
	  if(num==-1)goto _eofout;
	  s->lengthlist[i]=num+1;
	}else
	  s->lengthlist[i]=0;
      }
    }else{
      /* all entries used; no tagging */
      for(i=0;i<s->entries;i++){
	long num=oggpack_read(opb,5);
	if(num==-1)goto _eofout;
	s->lengthlist[i]=num+1;
      }
    }
    
    break;
  case 1:
    /* ordered */
    {
      long length=oggpack_read(opb,5)+1;
      s->lengthlist=_ogg_malloc(sizeof(*s->lengthlist)*s->entries);

      for(i=0;i<s->entries;){
	long num=oggpack_read(opb,_ilog(s->entries-i));
	if(num==-1)goto _eofout;
	for(j=0;j<num && i<s->entries;j++,i++)
	  s->lengthlist[i]=length;
	length++;
      }
    }
    break;
  default:
    /* EOF */
    return(-1);
  }
  
  /* Do we have a mapping to unpack? */
  switch((s->maptype=oggpack_read(opb,4))){
  case 0:
    /* no mapping */
    break;
  case 1: case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */

    s->q_min=oggpack_read(opb,32);
    s->q_delta=oggpack_read(opb,32);
    s->q_quant=oggpack_read(opb,4)+1;
    s->q_sequencep=oggpack_read(opb,1);

    {
      int quantvals=0;
      switch(s->maptype){
      case 1:
	quantvals=_book_maptype1_quantvals(s);
	break;
      case 2:
	quantvals=s->entries*s->dim;
	break;
      }
      
      /* quantized values */
      s->quantlist=_ogg_malloc(sizeof(*s->quantlist)*quantvals);
      for(i=0;i<quantvals;i++)
	s->quantlist[i]=oggpack_read(opb,s->q_quant);
      
      if(quantvals&&s->quantlist[quantvals-1]==-1)goto _eofout;
    }
    break;
  default:
    goto _errout;
  }

  /* all set */
  return(0);
  
 _errout:
 _eofout:
  vorbis_staticbook_clear(s);
  return(-1); 
}

/* returns the number of bits ************************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  oggpack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* One the encode side, our vector writers are each designed for a
specific purpose, and the encoder is not flexible without modification:

The LSP vector coder uses a single stage nearest-match with no
interleave, so no step and no error return.  This is specced by floor0
and doesn't change.

Residue0 encoding interleaves, uses multiple stages, and each stage
peels of a specific amount of resolution from a lattice (thus we want
to match by threshold, not nearest match).  Residue doesn't *have* to
be encoded that way, but to change it, one will need to add more
infrastructure on the encode side (decode side is specced and simpler) */

/* floor0 LSP (single stage, non interleaved, nearest match) */
/* returns entry number and *modifies a* to the quantization value *****/
int vorbis_book_errorv(codebook *book,float *a){
  int dim=book->dim,k;
  int best=_best(book,a,1);
  for(k=0;k<dim;k++)
    a[k]=(book->valuelist+best*dim)[k];
  return(best);
}

/* returns the number of bits and *modifies a* to the quantization value *****/
int vorbis_book_encodev(codebook *book,int best,float *a,oggpack_buffer *b){
  int k,dim=book->dim;
  for(k=0;k<dim;k++)
    a[k]=(book->valuelist+best*dim)[k];
  return(vorbis_book_encode(book,best,b));
}

/* the 'eliminate the decode tree' optimization actually requires the
   codewords to be MSb first, not LSb.  This is an annoying inelegancy
   (and one of the first places where carefully thought out design
   turned out to be wrong; Vorbis II and future Ogg codecs should go
   to an MSb bitpacker), but not actually the huge hit it appears to
   be.  The first-stage decode table catches most words so that
   bitreverse is not in the main execution path. */

static ogg_uint32_t bitreverse(ogg_uint32_t x){
  x=    ((x>>16)&0x0000ffff) | ((x<<16)&0xffff0000);
  x=    ((x>> 8)&0x00ff00ff) | ((x<< 8)&0xff00ff00);
  x=    ((x>> 4)&0x0f0f0f0f) | ((x<< 4)&0xf0f0f0f0);
  x=    ((x>> 2)&0x33333333) | ((x<< 2)&0xcccccccc);
  return((x>> 1)&0x55555555) | ((x<< 1)&0xaaaaaaaa);
}

STIN long decode_packed_entry_number(codebook *book, oggpack_buffer *b){
  int  read=book->dec_maxlength;
  long lo,hi;
  long lok = oggpack_look(b,book->dec_firsttablen);
 
  if (lok >= 0) {
    long entry = book->dec_firsttable[lok];
    if(entry&0x80000000UL){
      lo=(entry>>15)&0x7fff;
      hi=book->used_entries-(entry&0x7fff);
    }else{
      oggpack_adv(b, book->dec_codelengths[entry-1]);
      return(entry-1);
    }
  }else{
    lo=0;
    hi=book->used_entries;
  }

  lok = oggpack_look(b, read);

  while(lok<0 && read>1)
    lok = oggpack_look(b, --read);
  if(lok<0)return -1;

  /* bisect search for the codeword in the ordered list */
  {
    ogg_uint32_t testword=bitreverse((ogg_uint32_t)lok);

    while(hi-lo>1){
      long p=(hi-lo)>>1;
      long test=book->codelist[lo+p]>testword;    
      lo+=p&(test-1);
      hi-=p&(-test);
    }

    if(book->dec_codelengths[lo]<=read){
      oggpack_adv(b, book->dec_codelengths[lo]);
      return(lo);
    }
  }
  
  oggpack_adv(b, read);
  return(-1);
}

/* Decode side is specced and easier, because we don't need to find
   matches using different criteria; we simply read and map.  There are
   two things we need to do 'depending':
   
   We may need to support interleave.  We don't really, but it's
   convenient to do it here rather than rebuild the vector later.

   Cascades may be additive or multiplicitive; this is not inherent in
   the codebook, but set in the code using the codebook.  Like
   interleaving, it's easiest to do it here.  
   addmul==0 -> declarative (set the value)
   addmul==1 -> additive
   addmul==2 -> multiplicitive */

/* returns the [original, not compacted] entry number or -1 on eof *********/
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  long packed_entry=decode_packed_entry_number(book,b);
  if(packed_entry>=0)
    return(book->dec_index[packed_entry]);
  
  /* if there's no dec_index, the codebook unpacking isn't collapsed */
  return(packed_entry);
}

/* returns 0 on OK or -1 on eof *************************************/
long vorbis_book_decodevs_add(codebook *book,float *a,oggpack_buffer *b,int n){
  int step=n/book->dim;
  long *entry = alloca(sizeof(*entry)*step);
  float **t = alloca(sizeof(*t)*step);
  int i,j,o;

  for (i = 0; i < step; i++) {
    entry[i]=decode_packed_entry_number(book,b);
    if(entry[i]==-1)return(-1);
    t[i] = book->valuelist+entry[i]*book->dim;
  }
  for(i=0,o=0;i<book->dim;i++,o+=step)
    for (j=0;j<step;j++)
      a[o+j]+=t[j][i];
  return(0);
}

long vorbis_book_decodev_add(codebook *book,float *a,oggpack_buffer *b,int n){
  int i,j,entry;
  float *t;

  if(book->dim>8){
    for(i=0;i<n;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      for (j=0;j<book->dim;)
	a[i++]+=t[j++];
    }
  }else{
    for(i=0;i<n;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      j=0;
      switch((int)book->dim){
      case 8:
	a[i++]+=t[j++];
      case 7:
	a[i++]+=t[j++];
      case 6:
	a[i++]+=t[j++];
      case 5:
	a[i++]+=t[j++];
      case 4:
	a[i++]+=t[j++];
      case 3:
	a[i++]+=t[j++];
      case 2:
	a[i++]+=t[j++];
      case 1:
	a[i++]+=t[j++];
      case 0:
	break;
      }
    }
  }    
  return(0);
}

long vorbis_book_decodev_set(codebook *book,float *a,oggpack_buffer *b,int n){
  int i,j,entry;
  float *t;

  for(i=0;i<n;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    t     = book->valuelist+entry*book->dim;
    for (j=0;j<book->dim;)
      a[i++]=t[j++];
  }
  return(0);
}

long vorbis_book_decodevv_add(codebook *book,float **a,long offset,int ch,
			      oggpack_buffer *b,int n){
  long i,j,entry;
  int chptr=0;

  for(i=offset/ch;i<(offset+n)/ch;){
    entry = decode_packed_entry_number(book,b);
    if(entry==-1)return(-1);
    {
      const float *t = book->valuelist+entry*book->dim;
      for (j=0;j<book->dim;j++){
	a[chptr++][i]+=t[j];
	if(chptr==ch){
	  chptr=0;
	  i++;
	}
      }
    }
  }
  return(0);
}

#ifdef _V_SELFTEST
/* Simple enough; pack a few candidate codebooks, unpack them.  Code a
   number of vectors through (keeping track of the quantized values),
   and decode using the unpacked book.  quantized version of in should
   exactly equal out */

#include <stdio.h>

#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/res0a_13.vqh"
#define TESTSIZE 40

float test1[TESTSIZE]={
  0.105939f,
  0.215373f,
  0.429117f,
  0.587974f,

  0.181173f,
  0.296583f,
  0.515707f,
  0.715261f,

  0.162327f,
  0.263834f,
  0.342876f,
  0.406025f,

  0.103571f,
  0.223561f,
  0.368513f,
  0.540313f,

  0.136672f,
  0.395882f,
  0.587183f,
  0.652476f,

  0.114338f,
  0.417300f,
  0.525486f,
  0.698679f,

  0.147492f,
  0.324481f,
  0.643089f,
  0.757582f,

  0.139556f,
  0.215795f,
  0.324559f,
  0.399387f,

  0.120236f,
  0.267420f,
  0.446940f,
  0.608760f,

  0.115587f,
  0.287234f,
  0.571081f,
  0.708603f,
};

float test3[TESTSIZE]={
  0,1,-2,3,4,-5,6,7,8,9,
  8,-2,7,-1,4,6,8,3,1,-9,
  10,11,12,13,14,15,26,17,18,19,
  30,-25,-30,-1,-5,-32,4,3,-2,0};

static_codebook *testlist[]={&_vq_book_lsp20_0,
			     &_vq_book_res0a_13,NULL};
float   *testvec[]={test1,test3};

int main(){
  oggpack_buffer write;
  oggpack_buffer read;
  long ptr=0,i;
  oggpack_writeinit(&write);
  
  fprintf(stderr,"Testing codebook abstraction...:\n");

  while(testlist[ptr]){
    codebook c;
    static_codebook s;
    float *qv=alloca(sizeof(*qv)*TESTSIZE);
    float *iv=alloca(sizeof(*iv)*TESTSIZE);
    memcpy(qv,testvec[ptr],sizeof(*qv)*TESTSIZE);
    memset(iv,0,sizeof(*iv)*TESTSIZE);

    fprintf(stderr,"\tpacking/coding %ld... ",ptr);

    /* pack the codebook, write the testvector */
    oggpack_reset(&write);
    vorbis_book_init_encode(&c,testlist[ptr]); /* get it into memory
                                                  we can write */
    vorbis_staticbook_pack(testlist[ptr],&write);
    fprintf(stderr,"Codebook size %ld bytes... ",oggpack_bytes(&write));
    for(i=0;i<TESTSIZE;i+=c.dim){
      int best=_best(&c,qv+i,1);
      vorbis_book_encodev(&c,best,qv+i,&write);
    }
    vorbis_book_clear(&c);
    
    fprintf(stderr,"OK.\n");
    fprintf(stderr,"\tunpacking/decoding %ld... ",ptr);

    /* transfer the write data to a read buffer and unpack/read */
    oggpack_readinit(&read,oggpack_get_buffer(&write),oggpack_bytes(&write));
    if(vorbis_staticbook_unpack(&read,&s)){
      fprintf(stderr,"Error unpacking codebook.\n");
      exit(1);
    }
    if(vorbis_book_init_decode(&c,&s)){
      fprintf(stderr,"Error initializing codebook.\n");
      exit(1);
    }

    for(i=0;i<TESTSIZE;i+=c.dim)
      if(vorbis_book_decodev_set(&c,iv+i,&read,c.dim)==-1){
	fprintf(stderr,"Error reading codebook test data (EOP).\n");
	exit(1);
      }
    for(i=0;i<TESTSIZE;i++)
      if(fabs(qv[i]-iv[i])>.000001){
	fprintf(stderr,"read (%g) != written (%g) at position (%ld)\n",
		iv[i],qv[i],i);
	exit(1);
      }
	  
    fprintf(stderr,"OK\n");
    ptr++;
  }

  /* The above is the trivial stuff; now try unquantizing a log scale codebook */

  exit(0);
}

#endif
