#pragma once
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

#include <stdint.h>
#include <list>

#ifdef __GNUC__
  #define S_PACK __attribute__((__packed__))
  #define E_PACK
#else
  #define S_PACK __pragma(pack(push, 1))
  #define E_PACK __pragma(pack(pop))
#endif

#define MAX_IEC958_PACKET  24576
#define IEC958_DATA_OFFSET 8

#define DTS1_FRAME_SIZE   512
#define DTS2_FRAME_SIZE   1024
#define DTS3_FRAME_SIZE   2048
#define AC3_FRAME_SIZE    6144
#define EAC3_FRAME_SIZE   MAX_IEC958_PACKET
#define TRUEHD_FRAME_SIZE 6144

#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS 2
#define OUT_FRAMESTOBYTES(a) ((a) * OUT_CHANNELS * (OUT_SAMPLESIZE>>3))

class CAEPackIEC958
{
public:
  typedef int (*PackFunc)(uint8_t *data, unsigned int size, uint8_t *dest);

  static int PackAC3     (uint8_t *data, unsigned int size, uint8_t *dest);
  static int PackEAC3    (uint8_t *data, unsigned int size, uint8_t *dest);
  static int PackDTS_512 (uint8_t *data, unsigned int size, uint8_t *dest);
  static int PackDTS_1024(uint8_t *data, unsigned int size, uint8_t *dest);
  static int PackDTS_2048(uint8_t *data, unsigned int size, uint8_t *dest);
private:
  enum IEC958DataType
  {
    IEC958_TYPE_NULL   = 0x00,
    IEC958_TYPE_AC3    = 0x01,
    IEC958_TYPE_DTS1   = 0x0B, /*  512 samples */
    IEC958_TYPE_DTS2   = 0x0C, /* 1024 samples */
    IEC958_TYPE_DTS3   = 0x0D, /* 2048 samples */
    IEC958_TYPE_DTSHD  = 0x11,
    IEC958_TYPE_EAC3   = 0x15,
    IEC958_TYPE_TRUEHD = 0x16
  };

  S_PACK
  struct IEC958Packet
  {
    uint16_t m_preamble1;
    uint16_t m_preamble2;
    uint16_t m_type;
    uint16_t m_length;
    uint8_t  m_data[MAX_IEC958_PACKET - IEC958_DATA_OFFSET];
  };
  E_PACK
};

