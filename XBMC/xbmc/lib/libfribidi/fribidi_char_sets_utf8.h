/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#include "fribidi_config.h"
#ifndef FRIBIDI_NO_CHARSETS

#ifndef FRIBIDI_CHAR_SETS_UTF8_H
#define FRIBIDI_CHAR_SETS_UTF8_H

#include "fribidi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define fribidi_char_set_name_utf8 "UTF-8"
#define fribidi_char_set_title_utf8 "UTF-8 (Unicode)"
#define fribidi_char_set_desc_utf8 NULL
#define fribidi_char_set_enter_utf8 NULL
#define fribidi_char_set_leave_utf8 NULL

  int fribidi_unicode_to_utf8 (FriBidiChar *us, int length,
			       /* Output */
			       char *s);

/* warning: the length of output string may exceed the length of the input */
/* the length of the string is returned */
  int fribidi_utf8_to_unicode (char *s, int length,
			       /* Output */
			       FriBidiChar *us);

#ifdef	__cplusplus
}
#endif

#endif				/* FRIBIDI_CHAR_SETS_UTF8_H */

#endif
