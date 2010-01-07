/*
    $Id: testischar.c,v 1.1 2003/08/17 05:31:19 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>

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
/* Tests ISO9660 character sets. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <cdio/iso9660.h>

int
main (int argc, const char *argv[])
{
  int i, j;

  printf ("  ");

  for (j = 0; j < 0x10; j++)
    printf (" %1.1x", j);

  printf (" |");

  for (j = 0; j < 0x10; j++)
    printf (" %1.1x", j);

  printf ("\n");

  for (i = 0; i < 0x10; i++)
    {

      printf ("%1.1x ", i);

      for (j = 0; j < 0x10; j++)
	{
	  int c = (j << 4) + i;

	  printf (" %c", iso9660_isdchar (c) ? c : ' ');
	}

      printf (" |");

      for (j = 0; j < 0x10; j++)
	{
	  int c = (j << 4) + i;

	  printf (" %c", iso9660_isachar (c) ? c : ' ');
	}
      
      printf ("\n");
    }

  return 0;
}
