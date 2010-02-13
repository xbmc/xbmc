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

#include "IDVDAudioEncoder.h"
#include "Codecs/DllAvCodec.h"
#include "Codecs/DllAvFormat.h"

class CDVDAudioEncoderFFmpeg: public IDVDAudioEncoder
{
public:
  CDVDAudioEncoderFFmpeg();
  virtual ~CDVDAudioEncoderFFmpeg();
  virtual bool Initialize(unsigned int channels, enum PCMChannels *channelMap, unsigned int bitsPerSample, unsigned int sampleRate);
  virtual void Reset();

  /* returns this DSPs output format */
  virtual unsigned int      GetBitRate   ();
  virtual CodecID           GetCodecID   ();
  virtual unsigned int      GetPacketSize();

  /* add/get packets to/from the DSP */
  virtual int Encode (uint8_t *data, int size);
  virtual int GetData(uint8_t **data);
private:
  DllAvCodec m_dllAvCodec;
  DllAvUtil  m_dllAvUtil;

  AVCodecContext   *m_CodecCtx;
  enum PCMChannels  m_ChannelMap[PCM_MAX_CH];
  CPCMRemap         m_Remap;
  uint8_t          *m_Buffer;
  int               m_BufferSize;

  unsigned int      m_NeededFrames;
  unsigned int      m_NeededBytes;
  unsigned int      m_OutputBytes;
};

