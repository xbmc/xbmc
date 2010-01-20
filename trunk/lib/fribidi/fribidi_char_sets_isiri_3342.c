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

FriBidiChar fribidi_isiri_3342_to_unicode_tab[] = {	/* 0xA0-0xFF */
  0x0020, 0x200C, 0x200D, 0x0021, 0x00A4, 0x066A, 0x002E, 0x066C,
  0x0029, 0x0028, 0x00D7, 0x002B, 0x060C, 0x002D, 0x066B, 0x002F,
  0x06F0, 0x06F1, 0x06F2, 0x06F3, 0x06F4, 0x06F5, 0x06F6, 0x06F7,
  0x06F8, 0x06F9, 0x003A, 0x061B, 0x003C, 0x003D, 0x003E, 0x061F,
  0x0622, 0x0627, 0x0621, 0x0628, 0x067E, 0x062A, 0x062B, 0x062C,
  0x0686, 0x062D, 0x062E, 0x062F, 0x0630, 0x0631, 0x0632, 0x0698,
  0x0633, 0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063A,
  0x0641, 0x0642, 0x06A9, 0x06AF, 0x0644, 0x0645, 0x0646, 0x0648,
  0x0647, 0x06CC, 0x005D, 0x005B, 0x007D, 0x007B, 0x00AB, 0x00BB,
  /* 0xec,0xed,0xee,0xef not defined */
  0x002A, 0x0640, 0x007C, 0x005C, 0xec, 0xed, 0xee, 0xef,
  0x064E, 0x0650, 0x064F, 0x064B, 0x064D, 0x064C, 0x0651, 0x0652,
  0x0623, 0x0624, 0x0625, 0x0626, 0x0629, 0x0643, 0x064A, 0x007F
};

FriBidiChar
fribidi_isiri_3342_to_unicode_c (char sch)
{
  unsigned char ch = (unsigned char) sch;
  if ((ch >= 0x80 && ch <= 0xa0) || ch == 0xff)
    return (ch - 0x80);		/* FIXME: but they are strong typed RTL ! */
  else if (ch >= 0xa1 && ch <= 0xfe)
    return fribidi_isiri_3342_to_unicode_tab[ch - 0xa0];
  /* TODO */
  else
    return (char) ch;
}

int
fribidi_isiri_3342_to_unicode (char *s, int len, FriBidiChar *us)
{
  int i;

  for (i = 0; i < len + 1; i++)
    us[i] = fribidi_isiri_3342_to_unicode_c (s[i]);

  return len;
}

char
fribidi_unicode_to_isiri_3342_c (FriBidiChar uch)
{
  /* TODO */
  if (uch < 256)
    return (char) uch;
  else
    switch (uch)
      {
      case 0x060C:
	return (char) 0xAC;
      case 0x061B:
	return (char) 0xBB;
      case 0x061F:
	return (char) 0xBF;
      case 0x0621:
	return (char) 0xC2;
      case 0x0622:
	return (char) 0xC0;
      case 0x0623:
	return (char) 0xF8;
      case 0x0624:
	return (char) 0xF9;
      case 0x0625:
	return (char) 0xFA;
      case 0x0626:
	return (char) 0xFB;
      case 0x0627:
	return (char) 0xC1;
      case 0x0628:
	return (char) 0xC3;
      case 0x0629:
	return (char) 0xFC;
      case 0x062A:
	return (char) 0xC5;
      case 0x062B:
	return (char) 0xC6;
      case 0x062C:
	return (char) 0xC7;
      case 0x062D:
	return (char) 0xC9;
      case 0x062E:
	return (char) 0xCA;
      case 0x062F:
	return (char) 0xCB;
      case 0x0630:
	return (char) 0xCC;
      case 0x0631:
	return (char) 0xCD;
      case 0x0632:
	return (char) 0xCE;
      case 0x0633:
	return (char) 0xD0;
      case 0x0634:
	return (char) 0xD1;
      case 0x0635:
	return (char) 0xD2;
      case 0x0636:
	return (char) 0xD3;
      case 0x0637:
	return (char) 0xD4;
      case 0x0638:
	return (char) 0xD5;
      case 0x0639:
	return (char) 0xD6;
      case 0x063A:
	return (char) 0xD7;
      case 0x0640:
	return (char) 0xE9;
      case 0x0641:
	return (char) 0xD8;
      case 0x0642:
	return (char) 0xD9;
      case 0x0643:
	return (char) 0xFD;
      case 0x0644:
	return (char) 0xDC;
      case 0x0645:
	return (char) 0xDD;
      case 0x0646:
	return (char) 0xDE;
      case 0x0647:
	return (char) 0xE0;
      case 0x0648:
	return (char) 0xDF;
      case 0x064A:
	return (char) 0xFE;
      case 0x064B:
	return (char) 0xF3;
      case 0x064C:
	return (char) 0xF5;
      case 0x064D:
	return (char) 0xF4;
      case 0x064E:
	return (char) 0xF0;
      case 0x064F:
	return (char) 0xF2;
      case 0x0650:
	return (char) 0xF1;
      case 0x0651:
	return (char) 0xF6;
      case 0x0652:
	return (char) 0xF7;
      case 0x066A:
	return (char) 0xA5;
      case 0x066B:
	return (char) 0xAE;
      case 0x066C:
	return (char) 0xA7;
      case 0x067E:
	return (char) 0xC4;
      case 0x0686:
	return (char) 0xC8;
      case 0x0698:
	return (char) 0xCF;
      case 0x06A9:
	return (char) 0xDA;
      case 0x06AF:
	return (char) 0xDB;
      case 0x06CC:
	return (char) 0xE1;
      case 0x06F0:
	return (char) 0xB0;
      case 0x06F1:
	return (char) 0xB1;
      case 0x06F2:
	return (char) 0xB2;
      case 0x06F3:
	return (char) 0xB3;
      case 0x06F4:
	return (char) 0xB4;
      case 0x06F5:
	return (char) 0xB5;
      case 0x06F6:
	return (char) 0xB6;
      case 0x06F7:
	return (char) 0xB7;
      case 0x06F8:
	return (char) 0xB8;
      case 0x06F9:
	return (char) 0xB9;
      case 0x200C:
	return (char) 0xA1;
      case 0x200D:
	return (char) 0xA2;
      default:
	return '¿';
      }
}

int
fribidi_unicode_to_isiri_3342 (FriBidiChar *us, int length, char *s)
{
  int i;

  for (i = 0; i < length; i++)
    s[i] = fribidi_unicode_to_isiri_3342_c (us[i]);
  s[i] = 0;

  return length;
}

#endif
