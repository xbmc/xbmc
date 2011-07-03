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

#include <cassert>
#include "system.h"
#include "AEPackIEC61937.h"

#define IEC61937_PREAMBLE1  0xF872
#define IEC61937_PREAMBLE2  0x4E1F

inline void SwapEndian(uint16_t *dst, uint16_t *src, unsigned int size)
{
  for(unsigned int i = 0; i < size; ++i, ++dst, ++src)
    *dst = ((*src & 0xFF00) >> 8) | ((*src & 0x00FF) << 8);
}

int CAEPackIEC61937::PackAC3(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(AC3_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;

  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  if (data == NULL)
    data = packet->m_data;
  int bitstream_mode = data[5] & 0x7;
  packet->m_type      = IEC61937_TYPE_AC3 | (bitstream_mode << 8);
  packet->m_length    = size << 3;

#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(AC3_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(AC3_FRAME_SIZE);
}

int CAEPackIEC61937::PackEAC3(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(EAC3_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;

  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_EAC3;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(EAC3_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(EAC3_FRAME_SIZE);
}

int CAEPackIEC61937::PackDTS_512(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(DTS1_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;
  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_DTS1;
  packet->m_length    = size << 3;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(DTS1_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(DTS1_FRAME_SIZE);
}

int CAEPackIEC61937::PackDTS_1024(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(DTS2_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;
  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_DTS2;
  packet->m_length    = size << 3;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(DTS2_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(DTS2_FRAME_SIZE);
}

int CAEPackIEC61937::PackDTS_2048(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(DTS3_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;
  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_DTS3;
  packet->m_length    = size << 3;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(DTS3_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(DTS3_FRAME_SIZE);
}

int CAEPackIEC61937::PackTrueHD(uint8_t *data, unsigned int size, uint8_t *dest)
{
  assert(size <= OUT_FRAMESTOBYTES(TRUEHD_FRAME_SIZE));
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;
  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_TRUEHD;
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(TRUEHD_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return OUT_FRAMESTOBYTES(TRUEHD_FRAME_SIZE);
}

int CAEPackIEC61937::PackDTSHD(uint8_t *data, unsigned int size, uint8_t *dest, unsigned int subtype)
{
  struct IEC61937Packet *packet = (struct IEC61937Packet*)dest;
  packet->m_preamble1 = IEC61937_PREAMBLE1;
  packet->m_preamble2 = IEC61937_PREAMBLE2;
  packet->m_type      = IEC61937_TYPE_DTSHD | (subtype << 8);
  packet->m_length    = size;

  if (data == NULL)
    data = packet->m_data;
#ifdef __BIG_ENDIAN__
  else
    memcpy(packet->m_data, data, size);
#else
  SwapEndian((uint16_t*)packet->m_data, (uint16_t*)data, size >> 1);
#endif

  memset(packet->m_data + size, 0, OUT_FRAMESTOBYTES(TRUEHD_FRAME_SIZE) - IEC61937_DATA_OFFSET - size);
  return size; 
}

