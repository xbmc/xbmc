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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include "fribidi.h"

#include "fribidi_tab_mirroring.i"

FRIBIDI_API fribidi_boolean
fribidi_get_mirror_char (	/* Input */
			  FriBidiChar ch,
			  /* Output */
			  FriBidiChar *mirrored_ch)
{
  int pos, step;
  fribidi_boolean found;

  pos = step = (nFriBidiMirroredChars / 2) + 1;

  while (step > 1)
    {
      FriBidiChar cmp_ch = FriBidiMirroredChars[pos].ch;
      step = (step + 1) / 2;

      if (cmp_ch < ch)
	{
	  pos += step;
	  if (pos > nFriBidiMirroredChars - 1)
	    pos = nFriBidiMirroredChars - 1;
	}
      else if (cmp_ch > ch)
	{
	  pos -= step;
	  if (pos < 0)
	    pos = 0;
	}
      else
	break;
    }
  found = FriBidiMirroredChars[pos].ch == ch;
  if (mirrored_ch)
    *mirrored_ch = found ? FriBidiMirroredChars[pos].mirrored_ch : ch;

  return found;
}
