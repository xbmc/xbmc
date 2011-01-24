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

#include "fribidi.h"

/* the following added by Raphael Finkel <raphael@cs.uky.edu> 12/1999 */

int
fribidi_utf8_to_unicode (char *s, int len, FriBidiChar *us)
/* warning: the length of input string may exceed the length of the output */
{
  int length;
  char *t = s;

  length = 0;
  while (s - t < len)
    {
      if (*(unsigned char *) s <= 0x7f)	/* one byte */
	{
	  *us++ = *s++;		/* expand with 0s */
	}
      else if (*(unsigned char *) s <= 0xdf)	/* 2 byte */
	{
	  *us++ =
	    ((*(unsigned char *) s & 0x1f) << 6) +
	    ((*(unsigned char *) (s + 1)) & 0x3f);
	  s += 2;
	}
      else			/* 3 byte */
	{
	  *us++ =
	    ((int) (*(unsigned char *) s & 0x0f) << 12) +
	    ((*(unsigned char *) (s + 1) & 0x3f) << 6) +
	    (*(unsigned char *) (s + 2) & 0x3f);
	  s += 3;
	}
      length++;
    }
  *us = 0;
  return (length);
}

int
fribidi_unicode_to_utf8 (FriBidiChar *us, int length, char *s)
/* warning: the length of output string may exceed the length of the input */
{
  int i;
  char *t;

  t = s;
  for (i = 0; i < length; i++)
    {
      FriBidiChar mychar = us[i];
      if (mychar <= 0x7F)
	{			/* 7 sig bits */
	  *t++ = mychar;
	}
      else if (mychar <= 0x7FF)
	{			/* 11 sig bits */
	  *t++ = 0xC0 | (unsigned char) (mychar >> 6);	/* upper 5 bits */
	  *t++ = 0x80 | (unsigned char) (mychar & 0x3F);	/* lower 6 bits */
	}
      else if (mychar <= 0xFFFF)
	{			/* 16 sig bits */
	  *t++ = 0xE0 | (unsigned char) (mychar >> 12);	/* upper 4 bits */
	  *t++ = 0x80 | (unsigned char) ((mychar >> 6) & 0x3F);	/* next 6 bits */
	  *t++ = 0x80 | (unsigned char) (mychar & 0x3F);	/* lowest 6 bits */
	}
      else if (mychar < FRIBIDI_UNICODE_CHARS)
	{			/* 21 sig bits */
	  *t++ = 0xF0 | (unsigned char) ((mychar >> 18) & 0x07);	/* upper 3 bits */
	  *t++ = 0x80 | (unsigned char) ((mychar >> 12) & 0x3F);	/* next 6 bits */
	  *t++ = 0x80 | (unsigned char) ((mychar >> 6) & 0x3F);	/* next 6 bits */
	  *t++ = 0x80 | (unsigned char) (mychar & 0x3F);	/* lowest 6 bits */
	}
    }
  *t = 0;

  return (t - s);
}

#endif
