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

#include <string.h>
#include "fribidi.h"

#define ISO_HAMZA 0xc1
#define ISO_SUKUN 0xf2

#define UNI_HAMZA 0x0621
#define UNI_SUKUN 0x0652

FriBidiChar
fribidi_iso8859_6_to_unicode_c (char sch)
{
  unsigned char ch = (unsigned char) sch;
  if (ch >= ISO_HAMZA && ch <= ISO_SUKUN)
    return ch - ISO_HAMZA + UNI_HAMZA;
  else
    return ch;
}

int
fribidi_iso8859_6_to_unicode (char *s, int len, FriBidiChar *us)
{
  int i;

  for (i = 0; i < len + 1; i++)
    us[i] = fribidi_iso8859_6_to_unicode_c (s[i]);

  return len;
}

char
fribidi_unicode_to_iso8859_6_c (FriBidiChar uch)
{
  if (uch >= UNI_HAMZA && uch <= UNI_SUKUN)
    return (char) (uch - UNI_HAMZA + ISO_HAMZA);
  /* TODO: handle pre-composed and presentation chars */
  else if (uch < 256)
    return (char) uch;
  else if (uch == 0x060c)
    return (char) 0xac;
  else if (uch == 0x061b)
    return (char) 0xbb;
  else if (uch == 0x061f)
    return (char) 0xbf;
  else
    return '¿';
}

int
fribidi_unicode_to_iso8859_6 (FriBidiChar *us, int length, char *s)
{
  int i;

  for (i = 0; i < length; i++)
    s[i] = fribidi_unicode_to_iso8859_6_c (us[i]);
  s[i] = 0;

  return length;
}

#endif
