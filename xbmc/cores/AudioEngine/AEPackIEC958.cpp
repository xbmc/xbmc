/*
 *      Copyright (C) 2005-2010 Team XBMC
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

/* DTS spec shows it suppors both BE and LE, we should not need to convert */

#include "system.h"
#include "AEPackIEC958.h"

#define IEC958_PREAMBLE1  0xF872
#define IEC958_PREAMBLE2  0x4E1F

inline void SwapEndian(uint16_t *dst, uint16_t *src, unsigned int size)
{
  for(unsigned int i = 0; i < size; ++i, ++dst, ++src)
    *dst = ((*src & 0xFF00) >> 8) | ((*src & 0x00FF) << 8);
}

int CAEPackIEC958::PackAC3(uint8_t *data, unsigned int size, uint8_t *dest)
{
  struct IEC958Packet *packet = (struct IEC958Packet*)dest;

  packet->m_preamble1 = IEC958_PREAMBLE1;
  packet->m_preamble2 = IEC958_PREAMBLE2;
  packet->m_type      = IEC958_TYPE_AC3;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, sizeof(packet->m_data) - size);
  return sizeof(packet);
}

int CAEPackIEC958::PackDTS_512(uint8_t *data, unsigned int size, uint8_t *dest)
{
  struct IEC958Packet *packet = (struct IEC958Packet*)dest;
  packet->m_preamble1 = IEC958_PREAMBLE1;
  packet->m_preamble2 = IEC958_PREAMBLE2;
  packet->m_type      = IEC958_TYPE_DTS1;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, 512 - size);
  return 512;
}

int CAEPackIEC958::PackDTS_1024(uint8_t *data, unsigned int size, uint8_t *dest)
{
  struct IEC958Packet *packet = (struct IEC958Packet*)dest;
  packet->m_preamble1 = IEC958_PREAMBLE1;
  packet->m_preamble2 = IEC958_PREAMBLE2;
  packet->m_type      = IEC958_TYPE_DTS2;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, 1024 - size);
  return 1024;
}

int CAEPackIEC958::PackDTS_2048(uint8_t *data, unsigned int size, uint8_t *dest)
{
  struct IEC958Packet *packet = (struct IEC958Packet*)dest;
  packet->m_preamble1 = IEC958_PREAMBLE1;
  packet->m_preamble2 = IEC958_PREAMBLE2;
  packet->m_type      = IEC958_TYPE_DTS3;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, 2048 - size);
  return 2048;
}


