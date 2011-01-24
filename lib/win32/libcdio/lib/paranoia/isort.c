/*
  $Id: isort.c,v 1.4 2005/01/23 05:31:03 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/* sorted vector abstraction for paranoia */

/* Old isort got a bit complex.  This re-constrains complexity to
   give a go at speed through a more alpha-6-like mechanism. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "p_block.h"
#include "isort.h"

#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif

sort_info_t *
sort_alloc(long size)
{
  sort_info_t *ret=calloc(1, sizeof(sort_info_t));

  ret->vector=NULL;
  ret->sortbegin=-1;
  ret->size=-1;
  ret->maxsize=size;

  ret->head=calloc(65536,sizeof(sort_link *));
  ret->bucketusage=calloc(1, 65536*sizeof(long));
  ret->revindex=calloc(size,sizeof(sort_link));
  ret->lastbucket=0;

  return(ret);
}

void 
sort_unsortall(sort_info_t *i)
{
  if(i->lastbucket>2000){ /* a guess */
    memset(i->head,0,65536*sizeof(sort_link *));
  }else{
    long b;
    for(b=0;b<i->lastbucket;b++)
      i->head[i->bucketusage[b]]=NULL;
  }

  i->lastbucket=0;
  i->sortbegin=-1;
}

void 
sort_free(sort_info_t *i)
{
  free(i->revindex);
  free(i->head);
  free(i->bucketusage);
  free(i);
}
 
static void 
sort_sort(sort_info_t *i,long sortlo,long sorthi)
{
  long j;

  for(j=sorthi-1;j>=sortlo;j--){
    sort_link **hv=i->head+i->vector[j]+32768;
    sort_link *l=i->revindex+j;

    if(*hv==NULL){
      i->bucketusage[i->lastbucket]=i->vector[j]+32768;
      i->lastbucket++;
    }
    l->next=*hv;    
    *hv=l;
  }
  i->sortbegin=0;
}

/* size *must* be less than i->maxsize */
void 
sort_setup(sort_info_t *i, int16_t *vector, long *abspos, long size,
	   long sortlo, long sorthi)
{
  if(i->sortbegin!=-1)sort_unsortall(i);

  i->vector=vector;
  i->size=size;
  i->abspos=abspos;

  i->lo=min(size,max(sortlo-*abspos,0));
  i->hi=max(0,min(sorthi-*abspos,size));
}

sort_link *
sort_getmatch(sort_info_t *i, long post, long overlap, int value)
{
  sort_link *ret;

  if(i->sortbegin==-1)sort_sort(i,i->lo,i->hi);
  /* Now we reuse lo and hi */
  
  post=max(0,min(i->size,post));
  i->val=value+32768;
  i->lo=max(0,post-overlap);       /* absolute position */
  i->hi=min(i->size,post+overlap); /* absolute position */

  ret=i->head[i->val];
  while(ret){
    if(ipos(i,ret)<i->lo){
      ret=ret->next;
    }else{
      if(ipos(i,ret)>=i->hi)
	ret=NULL;
      break;
    }
  }
  /*i->head[i->val]=ret;*/
  return(ret);
}

sort_link *
sort_nextmatch(sort_info_t *i, sort_link *prev)
{
  sort_link *ret=prev->next;

  if(!ret || ipos(i,ret)>=i->hi)return(NULL); 
  return(ret);
}

