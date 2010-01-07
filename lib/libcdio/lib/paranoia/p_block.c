/*
    $Id: p_block.c,v 1.6 2005/01/23 05:31:03 rocky Exp $

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
#include <limits.h>
#include "p_block.h"
#include <cdio/cdda.h>
#include <cdio/paranoia.h>

#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif

linked_list *new_list(void *(*newp)(void),void (*freep)(void *))
{
  linked_list *ret=calloc(1,sizeof(linked_list));
  ret->new_poly=newp;
  ret->free_poly=freep;
  return(ret);
}

linked_element *add_elem(linked_list *l,void *elem)
{

  linked_element *ret=calloc(1,sizeof(linked_element));
  ret->stamp=l->current++;
  ret->ptr=elem;
  ret->list=l;

  if(l->head)
    l->head->prev=ret;
  else
    l->tail=ret;    
  ret->next=l->head;
  ret->prev=NULL;
  l->head=ret;
  l->active++;

  return(ret);
}

linked_element *
new_elem(linked_list *list)
{
  void *new=list->new_poly();
  return(add_elem(list,new));
}

void 
free_elem(linked_element *e,int free_ptr)
{
  linked_list *l=e->list;
  if(free_ptr)l->free_poly(e->ptr);

  if(e==l->head)
    l->head=e->next;
  if(e==l->tail)
    l->tail=e->prev;
    
  if(e->prev)
    e->prev->next=e->next;
  if(e->next)
    e->next->prev=e->prev;

  l->active--;
  free(e);
} 

void 
free_list(linked_list *list,int free_ptr)
{
  while(list->head)
    free_elem(list->head,free_ptr);
  free(list);
}

void *get_elem(linked_element *e)
{
  return(e->ptr);
}

linked_list *copy_list(linked_list *list)
{
  linked_list *list_new=new_list(list->new_poly,list->free_poly);
  linked_element *i=list->tail;

  while(i){
    add_elem(list_new,i->ptr);
    i=i->prev;
  }
  return(list_new);
}

/**** C_block stuff ******************************************************/

static c_block_t *
i_cblock_constructor(cdrom_paranoia_t *p)
{
  c_block_t *ret=calloc(1,sizeof(c_block_t));
  return(ret);
}

void 
i_cblock_destructor(c_block_t *c)
{
  if(c){
    if(c->vector)free(c->vector);
    if(c->flags)free(c->flags);
    c->e=NULL;
    free(c);
  }
}

c_block_t *
new_c_block(cdrom_paranoia_t *p)
{
  linked_element *e=new_elem(p->cache);
  c_block_t *c=e->ptr;
  c->e=e;
  c->p=p;
  return(c);
}

void free_c_block(c_block_t *c)
{
  /* also rid ourselves of v_fragments that reference this block */
  v_fragment *v=v_first(c->p);
  
  while(v){
    v_fragment *next=v_next(v);
    if(v->one==c)free_v_fragment(v);
    v=next;
  }    

  free_elem(c->e,1);
}

static v_fragment *
i_vfragment_constructor(void)
{
  v_fragment *ret=calloc(1,sizeof(v_fragment));
  return(ret);
}

static inline void 
i_v_fragment_destructor(v_fragment *v)
{
  free(v);
}

v_fragment *
new_v_fragment(cdrom_paranoia_t *p, c_block_t *one,
	       long int begin, long int end, int last)
{
  linked_element *e=new_elem(p->fragments);
  v_fragment *b=e->ptr;
  
  b->e=e;
  b->p=p;

  b->one=one;
  b->begin=begin;
  b->vector=one->vector+begin-one->begin;
  b->size=end-begin;
  b->lastsector=last;

  return(b);
}

void free_v_fragment(v_fragment *v)
{
  free_elem(v->e,1);
}

c_block_t *
c_first(cdrom_paranoia_t *p)
{
  if(p->cache->head)
    return(p->cache->head->ptr);
  return(NULL);
}

c_block_t *
c_last(cdrom_paranoia_t *p)
{
  if(p->cache->tail)
    return(p->cache->tail->ptr);
  return(NULL);
}

c_block_t *
c_next(c_block_t *c)
{
  if(c->e->next)
    return(c->e->next->ptr);
  return(NULL);
}

c_block_t *
c_prev(c_block_t *c)
{
  if(c->e->prev)
    return(c->e->prev->ptr);
  return(NULL);
}

v_fragment *
v_first(cdrom_paranoia_t *p)
{
  if(p->fragments->head){
    return(p->fragments->head->ptr);
  }
  return(NULL);
}

v_fragment *
v_last(cdrom_paranoia_t *p)
{
  if(p->fragments->tail)
    return(p->fragments->tail->ptr);
  return(NULL);
}

v_fragment *
v_next(v_fragment *v)
{
  if(v->e->next)
    return(v->e->next->ptr);
  return(NULL);
}

v_fragment *
v_prev(v_fragment *v)
{
  if(v->e->prev)
    return(v->e->prev->ptr);
  return(NULL);
}

void 
recover_cache(cdrom_paranoia_t *p)
{
  linked_list *l=p->cache;

  /* Are we at/over our allowed cache size? */
  while(l->active>p->cache_limit)
    /* cull from the tail of the list */
    free_c_block(c_last(p));

}

int16_t *
v_buffer(v_fragment *v)
{
  if(!v->one)return(NULL);
  if(!cv(v->one))return(NULL);
  return(v->vector);
}

/* alloc a c_block not on a cache list */
c_block_t *
c_alloc(int16_t *vector, long begin, long size)
{
  c_block_t *c=calloc(1,sizeof(c_block_t));
  c->vector=vector;
  c->begin=begin;
  c->size=size;
  return(c);
}

void c_set(c_block_t *v,long begin){
  v->begin=begin;
}

/* pos here is vector position from zero */
void 
c_insert(c_block_t *v,long pos,int16_t *b,long size)
{
  int vs=cs(v);
  if(pos<0 || pos>vs)return;

  if(v->vector) {
    v->vector = realloc(v->vector,sizeof(int16_t)*(size+vs));
  } else {
    v->vector = calloc(1, sizeof(int16_t)*size);
  }
  
  if(pos<vs)memmove(v->vector+pos+size,v->vector+pos,
		       (vs-pos)*sizeof(int16_t));
  memcpy(v->vector+pos,b,size*sizeof(int16_t));

  v->size+=size;
}

void 
c_remove(c_block_t *v, long cutpos, long cutsize)
{
  int vs=cs(v);
  if(cutpos<0 || cutpos>vs)return;
  if(cutpos+cutsize>vs)cutsize=vs-cutpos;
  if(cutsize<0)cutsize=vs-cutpos;
  if(cutsize<1)return;

  memmove(v->vector+cutpos,v->vector+cutpos+cutsize,
            (vs-cutpos-cutsize)*sizeof(int16_t));
  
  v->size-=cutsize;
}

void 
c_overwrite(c_block_t *v,long pos,int16_t *b,long size)
{
  int vs=cs(v);

  if(pos<0)return;
  if(pos+size>vs)size=vs-pos;

  memcpy(v->vector+pos,b,size*sizeof(int16_t));
}

void 
c_append(c_block_t *v, int16_t *vector, long size)
{
  int vs=cs(v);

  /* update the vector */
  if(v->vector)
    v->vector=realloc(v->vector,sizeof(int16_t)*(size+vs));
  else {
    v->vector=calloc(1, sizeof(int16_t)*size);
  }
  memcpy(v->vector+vs,vector,sizeof(int16_t)*size);

  v->size+=size;
}

void 
c_removef(c_block_t *v, long cut)
{
  c_remove(v,0,cut);
  v->begin+=cut;
}



/**** Initialization *************************************************/

/*!  Get the beginning and ending sector bounds given cursor position.

  There are a couple of subtle differences between this and the
  cdda_firsttrack_sector and cdda_lasttrack_sector. If the cursor is
  an a sector later than cdda_firsttrack_sector, that sectur will be
  used. As for the difference between cdda_lasttrack_sector, if the CD
  is mixed and there is a data track after the cursor but before the
  last audio track, the end of the audio sector before that is used.
*/
void 
i_paranoia_firstlast(cdrom_paranoia_t *p)
{
  track_t i, j;
  cdrom_drive_t *d=p->d;
  const track_t i_first_track = cdio_get_first_track_num(d->p_cdio);
  const track_t i_last_track  = cdio_get_last_track_num(d->p_cdio);

  p->current_lastsector = p->current_firstsector = -1;

  i = cdda_sector_gettrack(d, p->cursor); 

  if ( CDIO_INVALID_TRACK != i ) {
    if ( 0 == i ) i++;
    j = i;
    /* In the below loops, We assume the cursor already is on an audio
       sector. Not sure if this is correct if p->cursor is in the pregap
       before the first track.
    */
    for ( ; i < i_last_track; i++)
      if( !cdda_track_audiop(d,i) ) {
	p->current_lastsector=cdda_track_lastsector(d,i-1);
	break;
      }

    i = j;
    for ( ; i >= i_first_track; i-- )
      if( !cdda_track_audiop(d,i) ) {
	p->current_firstsector = cdda_track_firstsector(d,i+1);
	break;
      }
  }
  
  if (p->current_lastsector == -1)
    p->current_lastsector = cdda_disc_lastsector(d);

  if(p->current_firstsector == -1)
    p->current_firstsector = cdda_disc_firstsector(d);

}

cdrom_paranoia_t *
paranoia_init(cdrom_drive_t *d)
{
  cdrom_paranoia_t *p=calloc(1,sizeof(cdrom_paranoia_t));

  p->cache=new_list((void *)&i_cblock_constructor,
		    (void *)&i_cblock_destructor);

  p->fragments=new_list((void *)&i_vfragment_constructor,
			(void *)&i_v_fragment_destructor);

  p->readahead=150;
  p->sortcache=sort_alloc(p->readahead*CD_FRAMEWORDS);
  p->d=d;
  p->dynoverlap=MAX_SECTOR_OVERLAP*CD_FRAMEWORDS;
  p->cache_limit=JIGGLE_MODULO;
  p->enable=PARANOIA_MODE_FULL;
  p->cursor=cdda_disc_firstsector(d);
  p->lastread=LONG_MAX;

  /* One last one... in case data and audio tracks are mixed... */
  i_paranoia_firstlast(p);

  return(p);
}

