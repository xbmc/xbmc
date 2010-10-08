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

#include "AEEncoder.h"
#include "AERemap.h"

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict
#include "Codecs/DllAvCodec.h"
#include "Codecs/DllAvFormat.h"

class CAEEncoderFFmpeg: public IAEEncoder
{
public:
  CAEEncoderFFmpeg();
  virtual ~CAEEncoderFFmpeg();

  virtual bool Initialize(unsigned int channels, AEChLayout channelLayout, unsigned int sampleRate);
  virtual void Reset();

  virtual unsigned int GetBitRate    ();
  virtual CodecID      GetCodecID    ();
  virtual unsigned int GetFrames     ();
  virtual unsigned int GetEncodedSize();

  virtual int Encode (float *data, unsigned int frames);
  virtual int GetData(uint8_t **data);
private:
  DllAvCodec m_dllAvCodec;
  DllAvUtil  m_dllAvUtil;

  AVCodecContext   *m_CodecCtx;
  enum AEChannel    m_ChannelMap[AE_CH_MAX+1];
  CAERemap          m_Remap;
  uint8_t          *m_Buffer;
  float            *m_Remapped;
  int               m_BufferSize;

  unsigned int      m_NeededFrames;
};

