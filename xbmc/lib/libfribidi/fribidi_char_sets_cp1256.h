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

#ifndef FRIBIDI_CHAR_SETS_CP1256_H
#define FRIBIDI_CHAR_SETS_CP1256_H

#include "fribidi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define fribidi_char_set_name_cp1256 "CP1256"
#define fribidi_char_set_title_cp1256 "CP1256 (MS-Arabic)"
#define fribidi_char_set_desc_cp1256 NULL
#define fribidi_char_set_enter_cp1256 NULL
#define fribidi_char_set_leave_cp1256 NULL

  FriBidiChar fribidi_cp1256_to_unicode_c (char ch);
  int fribidi_cp1256_to_unicode (char *s, int length,
				 /* Output */
				 FriBidiChar *us);
  char fribidi_unicode_to_cp1256_c (FriBidiChar uch);
  int fribidi_unicode_to_cp1256 (FriBidiChar *us, int length,
				 /* Output */
				 char *s);

#ifdef	__cplusplus
}
#endif

#endif				/* FRIBIDI_CHAR_SETS_CP1256_H */

#endif
