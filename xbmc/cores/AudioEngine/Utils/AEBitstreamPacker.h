#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <list>
#include "AEPackIEC61937.h"

class CAEStreamInfo;

class CAEBitstreamPacker
{
public:
  CAEBitstreamPacker();
  ~CAEBitstreamPacker();

  void         Pack(CAEStreamInfo &info, uint8_t* data, int size);
  uint8_t*     GetBuffer();
  unsigned int GetSize  ();

private:
  void PackTrueHD(CAEStreamInfo &info, uint8_t* data, int size);
  void PackDTSHD (CAEStreamInfo &info, uint8_t* data, int size);

  /* we keep the trueHD and dtsHD buffers seperate so that we can handle a fast stream switch */
  uint8_t      *m_trueHD;
  unsigned int  m_trueHDPos;

  uint8_t      *m_dtsHD;
  unsigned int  m_dtsHDSize;

  unsigned int  m_dataSize;
  uint8_t       m_packedBuffer[MAX_IEC61937_PACKET];
};

