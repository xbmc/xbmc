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

#include <list>

#include "system.h"
#include "DVDAudioCodec.h"
#include "cores/AudioEngine/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEStreamInfo.h"
#include "cores/AudioEngine/Utils/AEBitstreamPacker.h"

class CDVDAudioCodecPassthrough : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthrough();
  virtual ~CDVDAudioCodecPassthrough();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int  Decode(BYTE* pData, int iSize);
  virtual int  GetData(BYTE** dst);
  virtual void Reset();
  virtual int  GetChannels               ();
  virtual int  GetEncodedChannels        ();
  virtual CAEChannelInfo GetChannelMap       ();
  virtual int  GetSampleRate             ();
  virtual int  GetEncodedSampleRate      ();
  virtual enum AEDataFormat GetDataFormat();
  virtual bool NeedPassthrough           () { return true;          }
  virtual const char* GetName            () { return "passthrough"; }
  virtual int  GetBufferSize();
private:
  CAEStreamInfo      m_info;
  CAEBitstreamPacker m_packer;
  uint8_t*           m_buffer;
  unsigned int       m_bufferSize;
};

