#pragma once
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

#include "DVDAudioTypes.h"

class CPCMSampleConverter
{
public:
  CPCMSampleConverter();
  ~CPCMSampleConverter();
  bool Initialize(DVDAudioFormat& inFormat, DVDAudioFormat& outFormat);
  bool AddFrame(DVDAudioFrame& frame);
  bool GetFrame(DVDAudioFrame& frame);
  float GetConversionFactor();
protected:
  void InitBuffer(unsigned int size, bool copyExisting = false);
  unsigned char* m_pBuffer;
  unsigned int m_BufferSize;
  unsigned int m_BufferOffset;
  DVDAudioFormat m_InputFormat;
  DVDAudioFormat m_OutputFormat;
};
