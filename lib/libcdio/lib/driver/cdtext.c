/*
    $Id: cdtext.c,v 1.2 2005/01/27 03:10:06 rocky Exp $

    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
    toc reading routine adapted from cuetools
    Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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
# include "config.h"
#endif

#include <cdio/cdtext.h>
#include <cdio/logging.h>
#include "cdtext_private.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef _XBOX
#include "xtl.h"
#else
#include <windows.h>
#endif

/*! Note: the order and number items (except CDTEXT_INVALID) should
  match the cdtext_field_t enumeration. */
const char *cdtext_keywords[] = 
  {
    "ARRANGER",
    "COMPOSER",
    "DISC_ID",
    "GENRE",
    "ISRC",
    "MESSAGE",
    "PERFORMER",
    "SIZE_INFO",
    "SONGWRITER",
    "TITLE",
    "TOC_INFO",
    "TOC_INFO2",
    "UPC_EAN",
  };


/*! Return string representation of the enum values above */
const char *
cdtext_field2str (cdtext_field_t i)
{
  if (i >= MAX_CDTEXT_FIELDS)
    return "Invalid CDTEXT field index";
  else 
    return cdtext_keywords[i];
}

/*! Free memory assocated with cdtext*/
void 
cdtext_destroy (cdtext_t *p_cdtext)
{
  cdtext_field_t i;

  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    if (p_cdtext->field[i]) {
      free(p_cdtext->field[i]);
      p_cdtext->field[i] = NULL;
    }
    
  }
}

/*! 
  returns the CDTEXT value associated with key. NULL is returned
  if key is CDTEXT_INVALID or the field is not set.
 */
char *
cdtext_get (cdtext_field_t key, const cdtext_t *p_cdtext)
{
  if (key == CDTEXT_INVALID) return NULL;
  return strdup(p_cdtext->field[key]);
}

/*! Initialize a new cdtext structure.
  When the structure is no longer needed, release the 
  resources using cdtext_delete.
*/
void 
cdtext_init (cdtext_t *p_cdtext)
{
  cdtext_field_t i;

  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    p_cdtext->field[i] = NULL;
  }
}

/*!
  returns 0 if field is a CD-TEXT keyword, returns non-zero otherwise 
*/
cdtext_field_t
cdtext_is_keyword (const char *key)
{
#if 0  
  char *item;
  
  item = bsearch(key, 
		 cdtext_keywords, 12,
		 sizeof (char *), 
		 (int (*)(const void *, const void *))
		 strcmp);
  return (NULL != item) ? 0 : 1;
#else 
  unsigned int i;
  
  for (i = 0; i < 13 ; i++)
    if (0 == strcmp (cdtext_keywords[i], key)) {
      return i;
    }
  return CDTEXT_INVALID;
#endif
}

/*! sets cdtext's keyword entry to field.
 */
void 
cdtext_set (cdtext_field_t key, const char *p_value, cdtext_t *p_cdtext)
{
  if (NULL == p_value || key == CDTEXT_INVALID) return;
  
  if (p_cdtext->field[key]) free (p_cdtext->field[key]);
  p_cdtext->field[key] = strdup (p_value);
  
}

#define SET_CDTEXT_FIELD(FIELD) \
  (*set_cdtext_field_fn)(p_user_data, i_track, i_first_track, FIELD, buffer);

/* 
  parse all CD-TEXT data retrieved.
*/       
bool
cdtext_data_init(void *p_user_data, track_t i_first_track, 
		 unsigned char *wdata, 
		 set_cdtext_field_fn_t set_cdtext_field_fn) 
{
  CDText_data_t *p_data;
  int           i;
  int           j;
  char          buffer[256];
  int           idx;
  int           i_track;
  bool          b_ret = false;
  
  memset( buffer, 0x00, sizeof(buffer) );
  idx = 0;
  
  p_data = (CDText_data_t *) (&wdata[4]);
  for( i=0; i < CDIO_CDTEXT_MAX_PACK_DATA; i++ ) {

#if TESTED
    if ( p_data->bDBC ) {
      cdio_warn("Double-byte characters not supported");
      return false;
    }
#endif
    
    if( p_data->seq != i )
      break;
    
    if( (p_data->type >= 0x80) 
	&& (p_data->type <= 0x85) && (p_data->block == 0) ) {
      i_track = p_data->i_track;
      
      for( j=0; j < CDIO_CDTEXT_MAX_TEXT_DATA; j++ ) {
	if( p_data->text[j] == 0x00 ) {
	  bool b_field_set=true;
	  switch( p_data->type) {
	  case CDIO_CDTEXT_TITLE: 
	    SET_CDTEXT_FIELD(CDTEXT_TITLE);
	    break;
	  case CDIO_CDTEXT_PERFORMER:  
	    SET_CDTEXT_FIELD(CDTEXT_PERFORMER);
	    break;
	  case CDIO_CDTEXT_SONGWRITER:
	    SET_CDTEXT_FIELD(CDTEXT_SONGWRITER);
	    break;
	  case CDIO_CDTEXT_COMPOSER:
	    SET_CDTEXT_FIELD(CDTEXT_COMPOSER);
	    break;
	  case CDIO_CDTEXT_ARRANGER:
	    SET_CDTEXT_FIELD(CDTEXT_ARRANGER);
	    break;
	  case CDIO_CDTEXT_MESSAGE:
	    SET_CDTEXT_FIELD(CDTEXT_MESSAGE);
	    break;
	  case CDIO_CDTEXT_DISCID: 
	    SET_CDTEXT_FIELD(CDTEXT_DISCID);
	    break;
	  case CDIO_CDTEXT_GENRE: 
	    SET_CDTEXT_FIELD(CDTEXT_GENRE);
	    break;
	  default : b_field_set = false;
	  }
	  if (b_field_set) {
	    b_ret = true;
	    i_track++;
	    idx = 0;
	  }
	} else {
	  buffer[idx++] = p_data->text[j];
	}
	buffer[idx] = 0x00;
      }
    }
    p_data++;
  }
  return b_ret;
}

