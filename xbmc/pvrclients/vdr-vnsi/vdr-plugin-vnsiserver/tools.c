/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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


uint64_t ntohll(uint64_t a)
{
  return htonll(a);
}

uint64_t htonll(uint64_t a)
{
#if BYTE_ORDER == BIG_ENDIAN
  return a;
#else
  uint64_t b = 0;

  b = ((a << 56) & 0xFF00000000000000ULL)
    | ((a << 40) & 0x00FF000000000000ULL)
    | ((a << 24) & 0x0000FF0000000000ULL)
    | ((a <<  8) & 0x000000FF00000000ULL)
    | ((a >>  8) & 0x00000000FF000000ULL)
    | ((a >> 24) & 0x0000000000FF0000ULL)
    | ((a >> 40) & 0x000000000000FF00ULL)
    | ((a >> 56) & 0x00000000000000FFULL) ;

  return b;
#endif
}
