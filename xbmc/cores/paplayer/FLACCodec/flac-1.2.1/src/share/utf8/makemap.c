/*
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <iconv.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  iconv_t cd;
  const char *ib;
  char *ob;
  size_t ibl, obl, k;
  unsigned char c, buf[4];
  int i, wc;

  if (argc != 2) {
    printf("Usage: %s ENCODING\n", argv[0]);
    printf("Output a charset map for the 8-bit ENCODING.\n");
    return 1;
  }

  cd = iconv_open("UCS-4", argv[1]);
  if (cd == (iconv_t)(-1)) {
    perror("iconv_open");
    return 1;
  }

  for (i = 0; i < 256; i++) {
    c = i;
    ib = &c;
    ibl = 1;
    ob = buf;
    obl = 4;
    k = iconv(cd, &ib, &ibl, &ob, &obl);
    if (!k && !ibl && !obl) {
      wc = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
      if (wc >= 0xffff) {
	printf("Dodgy value.\n");
	return 1;
      }
    }
    else if (k == (size_t)(-1) && errno == EILSEQ)
      wc = 0xffff;
    else {
      printf("Non-standard iconv.\n");
      return 1;
    }

    if (i % 8 == 0)
      printf("  ");
    printf("0x%04x", wc);
    if (i == 255)
      printf("\n");
    else if (i % 8 == 7)
      printf(",\n");
    else
      printf(", ");
  }

  return 0;
}
