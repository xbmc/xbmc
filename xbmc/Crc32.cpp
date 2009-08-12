/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*
 * Portion of this code was taken from efone.
 * efone - Distributed internet phone system.
 *
 * (c) 1999,2000 Krzysztof Dabrowski
 * (c) 1999,2000 ElysiuM deeZine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

/* based on implementation by Finn Yannick Jacobs */

#include "stdafx.h"
#include "Crc32.h"

#include <stdio.h>
#include <stdlib.h>

/* crc_tab[] -- this crcTable is being build by chksum_crc32GenTab().
 *              so make sure, you call it before using the other
 *              functions!
 */
u_int32_t crc_tab[256];

/* chksum_crc32gentab() --      to a global crc_tab[256], this one will
 *                              calculate the crcTable for crc32-checksums.
 *                              it is generated to the polynom [..]
 */
void chksum_crc32gentab()
{
   unsigned long crc, poly;
   int i, j;

   poly = 0xEDB88320L;
   for (i = 0; i < 256; i++)
   {
      crc = i;
      for (j = 8; j > 0; j--)
      {
         if (crc & 1)
         {
            crc = (crc >> 1) ^ poly;
         }
         else
         {
            crc >>= 1;
         }
      }
      crc_tab[i] = crc;
   }
}

/* chksum_crc() -- to a given block, this one calculates the
 *                              crc32-checksum until the length is
 *                              reached. the crc32-checksum will be
 *                              the result.
 */
u_int32_t chksum_crc32(unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   crc = 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}

/* Our constructor */
Crc32::Crc32()
{
    chksum_crc32gentab();
}

/* Destructor */
Crc32::~Crc32()
{}

/* Our public methods */
void Crc32::Compute(const void* buffer, unsigned int count)
{
  const unsigned char* ptr = (const unsigned char *) buffer;
  m_crc = (unsigned __int32)chksum_crc32((unsigned char *)ptr, count);
}

void Crc32::Compute(const CStdString& strValue)
{
  Compute(strValue.c_str(), strValue.size());
}

void Crc32::ComputeFromLowerCase(const CStdString& strValue)
{
  CStdString strLower = strValue;
  strLower.MakeLower();
  Compute(strLower.c_str(), strLower.size());
}
