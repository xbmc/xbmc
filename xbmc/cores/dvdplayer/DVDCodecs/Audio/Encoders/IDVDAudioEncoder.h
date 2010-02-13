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

#include "utils/PCMRemap.h"
#include "DVDStreamInfo.h"

class IDVDAudioEncoder
{
public:
  IDVDAudioEncoder() {};
  virtual ~IDVDAudioEncoder() {};
  virtual bool Initialize(unsigned int channels, enum PCMChannels *channelMap, unsigned int bitsPerSample, unsigned int sampleRate) = 0;
  virtual void Reset() = 0;

  /* returns this DSPs output format */
  virtual unsigned int GetBitRate   () = 0;
  virtual CodecID      GetCodecID   () = 0;
  virtual unsigned int GetPacketSize() = 0;

  /* add/get packets to/from the DSP */
  virtual int Encode (uint8_t *data, int size) = 0;
  virtual int GetData(uint8_t **data) = 0;
};

