/* Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Library.

   The GNU LIBICONV Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   The GNU LIBICONV Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU LIBICONV Library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, Inc., 51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Creates the beyond-BMP part of the GB18030.TXT reference table. */

#include <stdio.h>
#include <stdlib.h>

#include "binary-io.h"

int main ()
{
  int i1, i2, i3, i4, uc;

#if O_BINARY
  SET_BINARY(fileno(stdout));
#endif

  uc = 0x10000;
  for (i1 = 0x90; i1 <= 0xe3; i1++)
    for (i2 = 0x30; i2 <= 0x39; i2++)
      for (i3 = 0x81; i3 <= 0xfe; i3++)
        for (i4 = 0x30; i4 <= 0x39; i4++) {
          printf("0x%02X%02X%02X%02X\t0x%X\n", i1, i2, i3, i4, uc);
          uc++;
          if (uc == 0x110000)
            goto done;
        }
 done:

  if (ferror(stdout) || fclose(stdout))
    exit(1);
  exit(0);
}
