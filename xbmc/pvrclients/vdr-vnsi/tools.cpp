/*
 *      Copyright (C) 2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "tools.h"

#define TYP_INIT 0
#define TYP_SMLE 1
#define TYP_BIGE 2

uint64_t ntohll(uint64_t a)
{
  return htonll(a);
}

uint64_t htonll(uint64_t a) {
  static int typ = TYP_INIT;
  unsigned char c;
  union {
    uint64_t ull;
    unsigned char c[8];
  } x;
  if (typ == TYP_INIT) {
    x.ull = 0x01;
    typ = (x.c[7] == 0x01ULL) ? TYP_BIGE : TYP_SMLE;
  }
  if (typ == TYP_BIGE)
    return a;
  x.ull = a;
  c = x.c[0]; x.c[0] = x.c[7]; x.c[7] = c;
  c = x.c[1]; x.c[1] = x.c[6]; x.c[6] = c;
  c = x.c[2]; x.c[2] = x.c[5]; x.c[5] = c;
  c = x.c[3]; x.c[3] = x.c[4]; x.c[4] = c;
  return x.ull;
}
