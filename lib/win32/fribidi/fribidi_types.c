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

#ifdef DEBUG

char
fribidi_char_from_type (FriBidiCharType c)
{
  switch (c)
    {
    case FRIBIDI_TYPE_LTR:
      return 'L';
    case FRIBIDI_TYPE_RTL:
      return 'R';
    case FRIBIDI_TYPE_AL:
      return 'A';

    case FRIBIDI_TYPE_EN:
      return '1';
    case FRIBIDI_TYPE_AN:
      return '9';
    case FRIBIDI_TYPE_ES:
      return 'w';
    case FRIBIDI_TYPE_ET:
      return 'w';
    case FRIBIDI_TYPE_CS:
      return 'w';
    case FRIBIDI_TYPE_NSM:
      return '`';
    case FRIBIDI_TYPE_BN:
      return 'b';

    case FRIBIDI_TYPE_BS:
      return 'B';
    case FRIBIDI_TYPE_SS:
      return 'S';
    case FRIBIDI_TYPE_WS:
      return '_';
    case FRIBIDI_TYPE_ON:
      return 'n';

    case FRIBIDI_TYPE_LRE:
      return '+';
    case FRIBIDI_TYPE_RLE:
      return '+';
    case FRIBIDI_TYPE_LRO:
      return '+';
    case FRIBIDI_TYPE_RLO:
      return '+';
    case FRIBIDI_TYPE_PDF:
      return '-';

    default:
      return '?';
    }
};

#endif

char *
fribidi_type_name (FriBidiCharType c)
{
#define _FRIBIDI_CASE(type)	case FRIBIDI_TYPE_##type: return #type
  switch (c)
    {
      _FRIBIDI_CASE (LTR);
      _FRIBIDI_CASE (RTL);
      _FRIBIDI_CASE (AL);

      _FRIBIDI_CASE (EN);
      _FRIBIDI_CASE (AN);
      _FRIBIDI_CASE (ES);
      _FRIBIDI_CASE (ET);
      _FRIBIDI_CASE (CS);
      _FRIBIDI_CASE (NSM);
      _FRIBIDI_CASE (BN);

      _FRIBIDI_CASE (BS);
      _FRIBIDI_CASE (SS);
      _FRIBIDI_CASE (WS);
      _FRIBIDI_CASE (ON);

      _FRIBIDI_CASE (LRE);
      _FRIBIDI_CASE (RLE);
      _FRIBIDI_CASE (LRO);
      _FRIBIDI_CASE (RLO);
      _FRIBIDI_CASE (PDF);

      _FRIBIDI_CASE (SOT);
      _FRIBIDI_CASE (EOT);

    default:
      return "?";
    }
#undef _FRIBIDI_CASE
}

/* Map fribidi_prop_types to fribidi_types. */
const FriBidiCharType fribidi_prop_to_type[] = {
#define _FRIBIDI_ADD_TYPE(TYPE) FRIBIDI_TYPE_##TYPE,
#include "fribidi_types.i"
#undef _FRIBIDI_ADD_TYPE
};
