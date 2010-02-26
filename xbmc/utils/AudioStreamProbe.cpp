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

#include "AudioStreamProbe.h"

enum DVDAudioEncodingType CAudioStreamProbe::ProbeFormat(uint8_t *data, unsigned int size, unsigned int &bitRate)
{
  bool extended = false;

  if (ProbeAC3(data, size, bitRate, extended))
    return extended ? DVDAudioEncodingType_EAC3 : DVDAudioEncodingType_AC3;

  if (ProbeDTS(data, size, bitRate))
    return DVDAudioEncodingType_DTS;

  if (ProbeAAC(data, size, bitRate))
    return DVDAudioEncodingType_AAC;

  /* failed to probe the format */
  bitRate = 0;
  return DVDAudioEncodingType_None;
}

bool CAudioStreamProbe::ProbeAC3(uint8_t *data, unsigned int size, unsigned int &bitRate, bool &extended)
{
  /* we need atleast 6 bytes to properly probe for AC-3 */
  if (size < 6)
    return false;
    
  static const uint16_t bitrates   [] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
  static const uint16_t samplerates[] = {48000, 44100, 32000};
  static const uint8_t  eac3_blocks[] = {1, 2, 3, 6};
  	
  if (data[0] != 0x0b || data[1] != 0x77)
  return false;

  uint8_t bsid = data[5] >> 3;
  if (bsid > 16) return false;  

  uint8_t fscod = (data[4] & 0xc0) >> 6;
  if (bsid < 0xA) {
    /* AC-3 */
    uint8_t frmsizecod = data[4] & 0x3f;

    if (fscod == 3 || frmsizecod > 37)
      return false;

    extended = false;
    bitRate  = (bitrates[frmsizecod >> 1] * 1000) >> (bsid > 8 ? bsid - 8 : 0);
    return true;
  }

  /* EAC-3 */
  uint8_t frameType = (data[2] & 0xc0) >> 6;
  if (frameType == 3)
    return false;

  uint16_t frameSize = (((data[2] & 0x7) << 8 | data[3]) + 1) << 1;
  if (frameSize < 7)
    return false;

  uint16_t sampleRate;
  uint8_t  numBlocks = 6;

  if (fscod == 3) {
    fscod = (data[4] & 0x30) >> 4;
    if (fscod == 3)
      return false;
    sampleRate = samplerates[fscod] / 2;
  }
  else
  {
    numBlocks  = eac3_blocks[(data[4] & 0xc) >> 2];
    sampleRate = samplerates[fscod];
  }

  extended = true;
  bitRate  = (unsigned int)(8.0 * frameSize * sampleRate / (numBlocks * 256.0));
  return true;
}

bool CAudioStreamProbe::ProbeDTS(uint8_t *data, unsigned int size, unsigned int &bitRate)
{
  /* we need atleast 10 bytes to properly probe for DTS */
  if (size < 10)
    return false;
	
  static const uint16_t bitrates[29] = {
    32 , 56 , 64  , 96  , 112 , 128 , 192 , 224, 256             , 320 , 384 , 448 , 512 , 576 ,
    640, 768, 960, 1024, 1152, 1280, 1344, 1408, 1411/*,2 FIXME*/, 1472, 1536, 1920, 2048, 3072,
    3840
  };
	
  bool    littleEndian;
  uint8_t ratecode;
    
  /* 16bit le */ if (data[0] == 0x7F && data[1] == 0xFE && data[2] == 0x80 && data[3] == 0x01                                               ) littleEndian = true ; else
  /* 14bit le */ if (data[0] == 0x1F && data[1] == 0xFF && data[2] == 0xE8 && data[3] == 0x00 && data[4] == 0x07 && (data[5] & 0xF0) == 0xF0) littleEndian = true ; else
  /* 16bit be */ if (data[1] == 0x7F && data[0] == 0xFE && data[3] == 0x80 && data[2] == 0x01                                               ) littleEndian = false; else
  /* 14bit be */ if (data[1] == 0x1F && data[0] == 0xFF && data[3] == 0xE8 && data[2] == 0x00 && data[5] == 0x07 && (data[4] & 0xF0) == 0xF0) littleEndian = false; else
    return false;
  
  /* if it is not a termination frame the next 6 bits will be set */
  if (littleEndian) {
    if ((data[4] & 0x80) == 0x80 && (data[4] & 0x7C) != 0x7C)
      return false;
    ratecode = (data[8] & 0x3) << 3 | (data[9] & 0xe0) >> 5;
  }
  else
  {
    if ((data[5] & 0x80) == 0x80 && (data[5] & 0x7C) != 0x7C)
      return false;
    ratecode = (data[9] & 0x3) << 3 | (data[8] & 0xe0) >> 5;      
  }
  
  if (ratecode > 28)
    return false;
  
  bitRate = bitrates[ratecode] * 1000;
  return true;
}

bool CAudioStreamProbe::ProbeAAC(uint8_t *data, unsigned int size, unsigned int &bitRate)
{
  static const uint32_t samplerates[13] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
  if (data[0] != 0xff || (data[1] & 0xF0) != 0xf0)
    return false;

  uint8_t sr = (data[4] & 0xF0) >> 4;
  if (sr > 12)
    return false;

  uint16_t frameSize = (data[3] & 0x03) << 11 | data[4] << 3 | (data[5] & 0xE0) >> 5;
  if (frameSize < 7)
    return false;

  uint16_t samples = ((data[6] & 0x3) + 1) * 1024;
  bitRate = frameSize * 8 * samplerates[sr] / samples;
  return true;
}
