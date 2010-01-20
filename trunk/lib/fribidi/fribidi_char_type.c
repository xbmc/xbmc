/* FriBidi - Library of BiDi algorithm
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
 * For licensing issues, contact <fwpg@sharif.edu>. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "fribidi.h"

/*======================================================================
 *  fribidi_get_type() returns the bidi type of a character.
 *----------------------------------------------------------------------*/
FRIBIDI_API FriBidiCharType fribidi_get_type_internal (FriBidiChar uch);

FRIBIDI_API FriBidiCharType
fribidi_get_type (FriBidiChar uch)
{
  return fribidi_get_type_internal (uch);
}

FRIBIDI_API void
fribidi_get_types (		/* input */
		    FriBidiChar *str, FriBidiStrIndex len,
		    /* output */
		    FriBidiCharType *type)
{
  FriBidiStrIndex i;

  for (i = 0; i < len; i++)
    type[i] = fribidi_get_type (str[i]);
}

#ifdef MEM_OPTIMIZED

#if   HAS_FRIBIDI_TAB_CHAR_TYPE_9_I
#include "fribidi_tab_char_type_9.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_8_I
#include "fribidi_tab_char_type_8.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_7_I
#include "fribidi_tab_char_type_7.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_6_I
#include "fribidi_tab_char_type_6.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_5_I
#include "fribidi_tab_char_type_5.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_4_I
#include "fribidi_tab_char_type_4.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_3_I
#include "fribidi_tab_char_type_3.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_2_I
#include "fribidi_tab_char_type_2.i"
#else
#error You have no fribidi_tab_char_type_*.i file, please first make one by \
       make fribidi_tab_char_type_n.i which n is the compress level, a digit \
       between 2 and 9, or simply run make fribidi_tab_char_type_small, \
       retry to make.
#endif

#else

#if   HAS_FRIBIDI_TAB_CHAR_TYPE_2_I
#include "fribidi_tab_char_type_2.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_3_I
#include "fribidi_tab_char_type_3.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_4_I
#include "fribidi_tab_char_type_4.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_5_I
#include "fribidi_tab_char_type_5.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_6_I
#include "fribidi_tab_char_type_6.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_7_I
#include "fribidi_tab_char_type_7.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_8_I
#include "fribidi_tab_char_type_8.i"
#elif HAS_FRIBIDI_TAB_CHAR_TYPE_9_I
#include "fribidi_tab_char_type_9.i"
#else
#error You have no fribidi_tab_char_type_*.i file, please first make one by \
       make fribidi_tab_char_type_n.i which n is the compress level, a digit \
       between 2 and 9, or simply run make fribidi_tab_char_type_large, \
       retry to make.
#endif

#endif
