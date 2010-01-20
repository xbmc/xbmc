/*
    $Id: cdtext.hpp,v 1.1 2005/11/10 11:11:16 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file cdtext.hpp
 *  \brief methods relating to CD-Text information. This file
 *  should not be #included directly.
 */

/*! Return string representation of the enum values above */
const char *field2str (cdtext_field_t i) 
{
  return cdtext_field2str (i);
}

/*! returns an allocated string associated with the given field.  NULL is
  returned if key is CDTEXT_INVALID or the field is not set.
  
  The user needs to free the string when done with it.

  @see getConst to retrieve a constant string that doesn't
  have to be freed.
  
*/
char *get (cdtext_field_t key) 
{
  return cdtext_get (key, p_cdtext);
}

/*! returns the C cdtext_t pointer associated with this object. */
cdtext_t *get () 
{
  return p_cdtext;
}

/*! returns a const string associated with the given field.  NULL is
  returned if key is CDTEXT_INVALID or the field is not set.
  
  Don't use the string when the cdtext object (i.e. the CdIo_t object
  you got it from) is no longer valid.

  @see cdio_get to retrieve an allocated string that persists past the
  cdtext object.

*/
const char *getConst (cdtext_field_t key) 
{
  return cdtext_get_const (key, p_cdtext);
}

/*!
  returns enum of keyword if key is a CD-Text keyword, 
  returns MAX_CDTEXT_FIELDS non-zero otherwise.
*/
cdtext_field_t isKeyword (const char *key) 
{
  return cdtext_is_keyword (key);
}

/*! 
  sets cdtext's keyword entry to field 
*/
void set (cdtext_field_t key, const char *value) 
{
  cdtext_set (key, value, p_cdtext);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
