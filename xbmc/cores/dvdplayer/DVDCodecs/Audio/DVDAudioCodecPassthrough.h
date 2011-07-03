#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <list>

#include "system.h"
#include "DVDAudioCodec.h"
#include "cores/AudioEngine/AEAudioFormat.h"
#include "cores/AudioEngine/AEStreamInfo.h"
#include "cores/AudioEngine/AEPackIEC61937.h"

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
  virtual AEChLayout GetChannelMap       ();
  virtual int  GetSampleRate             ();
  virtual int  GetEncodedSampleRate      ();
  virtual enum AEDataFormat GetDataFormat();
  virtual bool NeedPassthrough           () { return true;          }
  virtual const char* GetName            () { return "passthrough"; }
private:
  CAEStreamInfo m_info;

  unsigned int  m_channels;

  uint8_t      *m_buffer;
  unsigned int  m_bufferSize;

  /* we keep the trueHD and dtsHD buffers seperate so that we can handle a fast stream switch */
  unsigned int  m_trueHDPos;
  uint8_t      *m_trueHD;

  unsigned int  m_dtsHDSize;
  unsigned int  m_dtsHDPos;
  uint8_t      *m_dtsHD;

  unsigned int  m_dataSize;
  uint8_t       m_packedBuffer[MAX_IEC61937_PACKET];

  void PackTrueHD(uint8_t *data, int size);
  void PackDTSHD (uint8_t *data, int size);
};

