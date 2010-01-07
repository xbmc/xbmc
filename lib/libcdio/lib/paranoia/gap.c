/*
  $Id: gap.c,v 1.1 2004/12/18 17:29:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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
/***
 * Gap analysis support code for paranoia
 *
 ***/

#include "config.h"
#include "p_block.h"
#include <cdio/paranoia.h>
#include "gap.h"
#include <string.h>

/**** Gap analysis code ***************************************************/

long int
i_paranoia_overlap_r(int16_t *buffA,int16_t *buffB,
			  long offsetA, long offsetB)
{
  long beginA=offsetA;
  long beginB=offsetB;

  for( ; beginA>=0 && beginB>=0; beginA--,beginB-- )
    if (buffA[beginA] != buffB[beginB]) break;
  beginA++;
  beginB++;
  
  return(offsetA-beginA);
}

long int 
i_paranoia_overlap_f(int16_t *buffA,int16_t *buffB,
		     long offsetA, long offsetB,
		     long sizeA,long sizeB)
{
  long endA=offsetA;
  long endB=offsetB;
  
  for(;endA<sizeA && endB<sizeB;endA++,endB++)
    if(buffA[endA]!=buffB[endB])break;
  
  return(endA-offsetA);
}

int 
i_stutter_or_gap(int16_t *A, int16_t *B,long offA, long offB, long int gap)
{
  long a1=offA;
  long b1=offB;
  
  if(a1<0){
    b1-=a1;
    gap+=a1;
    a1=0;
  }
  
  return(memcmp(A+a1,B+b1,gap*2));
}

/* riftv is the first value into the rift -> or <- */
void 
i_analyze_rift_f(int16_t *A,int16_t *B,
		 long sizeA, long sizeB,
		 long aoffset, long boffset, 
		 long *matchA,long *matchB,long *matchC)
{
  
  long apast=sizeA-aoffset;
  long bpast=sizeB-boffset;
  long i;
  
  *matchA=0, *matchB=0, *matchC=0;
  
  /* Look for three possible matches... (A) Ariftv->B, (B) Briftv->A and 
     (c) AB->AB. */
  
  for(i=0;;i++){
    if(i<bpast) /* A */
      if(i_paranoia_overlap_f(A,B,aoffset,boffset+i,sizeA,sizeB)>=MIN_WORDS_RIFT){
	*matchA=i;
	break;
      }
    
    if(i<apast){ /* B */
      if(i_paranoia_overlap_f(A,B,aoffset+i,boffset,sizeA,sizeB)>=MIN_WORDS_RIFT){
	*matchB=i;
	break;
      }
      if(i<bpast) /* C */
	if(i_paranoia_overlap_f(A,B,aoffset+i,boffset+i,sizeA,sizeB)>=MIN_WORDS_RIFT){
	  *matchC=i;
	  break;
	}
    }else
      if(i>=bpast)break;
    
  }
  
  if(*matchA==0 && *matchB==0 && *matchC==0)return;
  
  if(*matchC)return;
  if(*matchA){
    if(i_stutter_or_gap(A,B,aoffset-*matchA,boffset,*matchA))
      return;
    *matchB=-*matchA; /* signify we need to remove n bytes from B */
    *matchA=0;
    return;
  }else{
    if(i_stutter_or_gap(B,A,boffset-*matchB,aoffset,*matchB))
      return;
    *matchA=-*matchB;
    *matchB=0;
    return;
  }
}

/* riftv must be first even val of rift moving back */

void 
i_analyze_rift_r(int16_t *A,int16_t *B,
		 long sizeA, long sizeB,
		 long aoffset, long boffset, 
		 long *matchA,long *matchB,long *matchC)
{
  
  long apast=aoffset+1;
  long bpast=boffset+1;
  long i;
  
  *matchA=0, *matchB=0, *matchC=0;
  
  /* Look for three possible matches... (A) Ariftv->B, (B) Briftv->A and 
     (c) AB->AB. */
  
  for(i=0;;i++){
    if(i<bpast) /* A */
      if(i_paranoia_overlap_r(A,B,aoffset,boffset-i)>=MIN_WORDS_RIFT){
	*matchA=i;
	break;
      }
    if(i<apast){ /* B */
      if(i_paranoia_overlap_r(A,B,aoffset-i,boffset)>=MIN_WORDS_RIFT){
	*matchB=i;
	break;
      }      
      if(i<bpast) /* C */
	if(i_paranoia_overlap_r(A,B,aoffset-i,boffset-i)>=MIN_WORDS_RIFT){
	  *matchC=i;
	  break;
	}
    }else
      if(i>=bpast)break;
    
  }
  
  if(*matchA==0 && *matchB==0 && *matchC==0)return;
  
  if(*matchC)return;
  
  if(*matchA){
    if(i_stutter_or_gap(A,B,aoffset+1,boffset-*matchA+1,*matchA))
      return;
    *matchB=-*matchA; /* signify we need to remove n bytes from B */
    *matchA=0;
    return;
  }else{
    if(i_stutter_or_gap(B,A,boffset+1,aoffset-*matchB+1,*matchB))
      return;
    *matchA=-*matchB;
    *matchB=0;
    return;
  }
}

void
analyze_rift_silence_f(int16_t *A,int16_t *B,long sizeA,long sizeB,
		       long aoffset, long boffset,
		       long *matchA, long *matchB)
{
  *matchA=-1;
  *matchB=-1;

  sizeA=min(sizeA,aoffset+MIN_WORDS_RIFT);
  sizeB=min(sizeB,boffset+MIN_WORDS_RIFT);

  aoffset++;
  boffset++;

  while(aoffset<sizeA){
    if(A[aoffset]!=A[aoffset-1]){
      *matchA=0;
      break;
    }
    aoffset++;
  }

  while(boffset<sizeB){
    if(B[boffset]!=B[boffset-1]){
      *matchB=0;
      break;
    }
    boffset++;
  }
}
