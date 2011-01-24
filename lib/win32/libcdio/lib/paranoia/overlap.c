/*
  $Id: overlap.c,v 1.3 2005/01/07 02:42:29 rocky Exp $

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
/***
 *
 * Statistic code and cache management for overlap settings
 *
 ***/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <cdio/paranoia.h>
#include "p_block.h"
#include "overlap.h"
#include "isort.h"

#ifdef _XBOX
#include "xtl.h"
#endif

/**** Internal cache management *****************************************/

void 
paranoia_resetcache(cdrom_paranoia_t *p)
{
  c_block_t *c=c_first(p);
  v_fragment *v;

  while(c){
    free_c_block(c);
    c=c_first(p);
  }

  v=v_first(p);
  while(v){
    free_v_fragment(v);
    v=v_first(p);
  }
}

void 
paranoia_resetall(cdrom_paranoia_t *p)
{
  p->root.returnedlimit=0;
  p->dyndrift=0;
  p->root.lastsector=0;

  if(p->root.vector){
    i_cblock_destructor(p->root.vector);
    p->root.vector=NULL;
  }

  paranoia_resetcache(p);
}

void 
i_paranoia_trim(cdrom_paranoia_t *p, long int beginword, long int endword)
{
  root_block *root=&(p->root);
  if(root->vector!=NULL){
    long target=beginword-MAX_SECTOR_OVERLAP*CD_FRAMEWORDS;
    long rbegin=cb(root->vector);
    long rend=ce(root->vector);

    if(rbegin>beginword)
      goto rootfree;
    
    if(rbegin+MAX_SECTOR_OVERLAP*CD_FRAMEWORDS<beginword){
      if(target+MIN_WORDS_OVERLAP>rend)
	goto rootfree;

      {
	long int offset=target-rbegin;
	c_removef(root->vector,offset);
      }
    }

    {
      c_block_t *c=c_first(p);
      while(c){
	c_block_t *next=c_next(c);
	if(ce(c)<beginword-MAX_SECTOR_OVERLAP*CD_FRAMEWORDS)
	  free_c_block(c);
	c=next;
      }
    }

  }
  return;
  
rootfree:

  i_cblock_destructor(root->vector);
  root->vector=NULL;
  root->returnedlimit=-1;
  root->lastsector=0;
  
}

/**** Statistical and heuristic[al? :-] management ************************/

void 
offset_adjust_settings(cdrom_paranoia_t *p, 
		       void(*callback)(long int, paranoia_cb_mode_t))
{
  if(p->stage2.offpoints>=10){
    /* drift: look at the average offset value.  If it's over one
       sector, frob it.  We just want a little hysteresis [sp?]*/
    long av=(p->stage2.offpoints?p->stage2.offaccum/p->stage2.offpoints:0);
    
    if(abs(av)>p->dynoverlap/4){
      av=(av/MIN_SECTOR_EPSILON)*MIN_SECTOR_EPSILON;
      
      if(callback)(*callback)(ce(p->root.vector),PARANOIA_CB_DRIFT);
      p->dyndrift+=av;
      
      /* Adjust all the values in the cache otherwise we get a
	 (potentially unstable) feedback loop */
      {
	c_block_t *c=c_first(p);
	v_fragment *v=v_first(p);

	while(v && v->one){
	  /* safeguard beginning bounds case with a hammer */
	  if(fb(v)<av || cb(v->one)<av){
	    v->one=NULL;
	  }else{
	    fb(v)-=av;
	  }
	  v=v_next(v);
	}
	while(c){
	  long adj=min(av,cb(c));
	  c_set(c,cb(c)-adj);
	  c=c_next(c);
	}
      }

      p->stage2.offaccum=0;
      p->stage2.offmin=0;
      p->stage2.offmax=0;
      p->stage2.offpoints=0;
      p->stage2.newpoints=0;
      p->stage2.offdiff=0;
    }
  }

  if(p->stage1.offpoints>=10){
    /* dynoverlap: we arbitrarily set it to 4x the running difference
       value, unless min/max are more */

    p->dynoverlap=(p->stage1.offpoints?p->stage1.offdiff/
		   p->stage1.offpoints*3:CD_FRAMEWORDS);

    if(p->dynoverlap<-p->stage1.offmin*1.5)
      p->dynoverlap=-p->stage1.offmin*1.5;
						     
    if(p->dynoverlap<p->stage1.offmax*1.5)
      p->dynoverlap=p->stage1.offmax*1.5;

    if(p->dynoverlap<MIN_SECTOR_EPSILON)p->dynoverlap=MIN_SECTOR_EPSILON;
    if(p->dynoverlap>MAX_SECTOR_OVERLAP*CD_FRAMEWORDS)
      p->dynoverlap=MAX_SECTOR_OVERLAP*CD_FRAMEWORDS;
    			     
    if(callback)(*callback)(p->dynoverlap,PARANOIA_CB_OVERLAP);

    if(p->stage1.offpoints>600){ /* bit of a bug; this routine is
				    called too often due to the overlap 
				    mesh alg we use in stage 1 */
      p->stage1.offpoints/=1.2;
      p->stage1.offaccum/=1.2;
      p->stage1.offdiff/=1.2;
    }
    p->stage1.offmin=0;
    p->stage1.offmax=0;
    p->stage1.newpoints=0;
  }
}

void 
offset_add_value(cdrom_paranoia_t *p,offsets *o,long value,
		 void(*callback)(long int, paranoia_cb_mode_t))
{
  if(o->offpoints!=-1){

    o->offdiff+=abs(value);
    o->offpoints++;
    o->newpoints++;
    o->offaccum+=value;
    if(value<o->offmin)o->offmin=value;
    if(value>o->offmax)o->offmax=value;
    
    if(o->newpoints>=10)offset_adjust_settings(p,callback);
  }
}

