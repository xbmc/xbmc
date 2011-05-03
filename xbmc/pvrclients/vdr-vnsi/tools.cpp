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

/*
 * Most of this code is taken from tools.c in the Video Disk Recorder ('VDR')
 */

#include "tools.h"
#include "libPlatform/os-dependent.h"
#include "client.h"

/* Byte order (just for windows)*/
#ifdef __WINDOWS__
#undef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#define BYTE_ORDER LITTLE_ENDIAN
#endif

uint64_t ntohll(uint64_t a)
{
  return htonll(a);
}

uint64_t htonll(uint64_t a)
{
#if (BYTE_ORDER == BIG_ENDIAN)
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

// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
  Set(Ms);
}

uint64_t cTimeMs::Now(void)
{
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  return Now() - begin;
}
