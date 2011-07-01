#pragma once
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

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <string.h>

#ifdef __SSE__
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include "AEAudioFormat.h"
#include "utils/MathUtils.h"

/* note: always converts to machine byte endian */

#ifdef CLAMP
#undef CLAMP
#endif
#define CLAMP(x) std::min(-1.0f, std::max(1.0f, (float)(x)))

#ifdef  INT24_MAX
#undef  INT24_MAX 
#endif
#define INT24_MAX 0x7FFFFF

#ifdef  INT16_MAX
#undef  INT16_MAX
#endif
#define INT16_MAX 0x7FFF

static inline int safeRound(double f)
{
  /* if the value is larger then we can handle, then clamp it */
  if (f >= INT_MAX) return INT_MAX;
  if (f <= INT_MIN) return INT_MIN;
  
  /* if the value is out of the MathUtils::round_int range, then round it normally */
  if (f <= static_cast<double>(INT_MIN / 2) - 1.0 || f >= static_cast <double>(INT_MAX / 2) + 1.0)
    return floor(f+0.5);
  
  return MathUtils::round_int(f);
}

class CAEConvert{
private:  
  static unsigned int U8_Float    (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S8_Float    (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S16LE_Float (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S16BE_Float (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S24LE4_Float(uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S24BE4_Float(uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S24LE3_Float(uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S24BE3_Float(uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S32LE_Float (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int S32BE_Float (uint8_t *data, const unsigned int samples, float   *dest);
  static unsigned int DOUBLE_Float(uint8_t *data, const unsigned int samples, float   *dest);

  static unsigned int Float_U8    (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S8    (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S16LE (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S16BE (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S24NE4(float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S24NE3(float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S32LE (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_S32BE (float   *data, const unsigned int samples, uint8_t *dest);
  static unsigned int Float_DOUBLE(float   *data, const unsigned int samples, uint8_t *dest);
public:
  typedef unsigned int (*AEConvertToFn)(uint8_t *data, const unsigned int samples, float   *dest);
  typedef unsigned int (*AEConvertFrFn)(float   *data, const unsigned int samples, uint8_t *dest);

  static AEConvertToFn ToFloat(enum AEDataFormat dataFormat);
  static AEConvertFrFn FrFloat(enum AEDataFormat dataFormat);
};

