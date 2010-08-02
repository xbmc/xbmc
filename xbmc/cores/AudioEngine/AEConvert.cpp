/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://xbmc.org
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

#include "AEConvert.h"
#include "AEUtil.h"
#include "AELookupU8.h"
#include "AELookupS16.h"

#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define LITTE_ENDIAN

CAEConvert::AEConvertToFn CAEConvert::ToFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8   : return &U8_Float;
    case AE_FMT_S16LE: return &S16LE_Float;
    case AE_FMT_S16BE: return &S16BE_Float;
    case AE_FMT_S24BE: return &S24BE_Float;
    default:
      return NULL;
  }
}

CAEConvert::AEConvertFrFn CAEConvert::FrFloat(enum AEDataFormat dataFormat)
{
  switch(dataFormat)
  {
    case AE_FMT_U8   : return &Float_U8;
    case AE_FMT_S16LE: return &Float_S16LE;
    case AE_FMT_S16BE: return &Float_S16BE;
    default:
      return NULL;
  }
}

unsigned int CAEConvert::U8_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
    *dest = AELookupU8toFloat[*data];

  return samples;
}

unsigned int CAEConvert::S16LE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifdef LITTE_ENDIAN
    *dest = AELookupS16toFloat[*(int16_t*)data + 32768];
#else
    int16_t value;
    swab(data, &value, 2);
    *dest = AELookupS16toFloat[value + 32768];
#endif
  }

  return samples;
}

unsigned int CAEConvert::S16BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, data += 2, ++dest)
  {
#ifdef LITTLE_ENDIAN
    int16_t value;
    swab(data, &value, 2);
    *(int32_t*)dest = AELookupS16toFloat[value + 32768];
#else
    *(int32_t*)dest = AELookupS16toFloat[*(int16_t*)data + 32768];
#endif
  }

  return samples;
}

unsigned int CAEConvert::S24BE_Float(uint8_t *data, const unsigned int samples, float *dest)
{
  unsigned int i;
  unsigned int viable = samples / 12;

  /* http://wiki.multimedia.cx/index.php?title=PCM#24-Bit_PCM */
  for(i = 0; i < viable; i += 4, data += 12, dest += 4)
  {
    dest[0] = ((data[0] << 16) | (data[1] << 8) | data[ 8]) / 167772155.0f;
    dest[1] = ((data[2] << 16) | (data[3] << 8) | data[ 9]) / 167772155.0f;
    dest[2] = ((data[4] << 16) | (data[5] << 8) | data[10]) / 167772155.0f;
    dest[3] = ((data[6] << 16) | (data[7] << 8) | data[11]) / 167772155.0f;
  }

  return viable;
}

unsigned int CAEConvert::Float_U8(float *data, const unsigned int samples, uint8_t *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
    *dest = (*data + 1.0f) * 127.5f;

  return i;
}

unsigned int CAEConvert::Float_S16LE(float *data, const unsigned int samples, uint8_t *dest)
{
  unsigned int i;
  int16_t *d = (int16_t*)dest;

  for(i = 0; i < samples; ++i, ++data, ++d)
  {
#ifdef LITTLE_ENDIAN
    *d = round(*data * 32767.5f);
#else
    int16_t value = *data * 32767.5f;
    swab(&value, d, 2);
#endif
  }

  return i * 2;
}

unsigned int CAEConvert::Float_S16BE(float *data, const unsigned int samples, uint8_t *dest)
{
  unsigned int i;
  for(i = 0; i < samples; ++i, ++data, ++dest)
  {
#ifdef LITTLE_ENDIAN
    int16_t value = *data * 32767.5f;
    swab(&value, dest, 2);
#else
    *(int16_t*)dest = *data * 32767.5f;
#endif
  }

  return i * 2;
}

