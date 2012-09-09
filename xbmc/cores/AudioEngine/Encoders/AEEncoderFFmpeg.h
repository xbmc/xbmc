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

#include "Interfaces/AEEncoder.h"
#include "Utils/AERemap.h"
#include "Utils/AEPackIEC61937.h"

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict
#include "DllAvCodec.h"
#include "DllAvFormat.h"

class CAEEncoderFFmpeg: public IAEEncoder
{
public:
  CAEEncoderFFmpeg();
  virtual ~CAEEncoderFFmpeg();

  virtual bool IsCompatible(AEAudioFormat format);
  virtual bool Initialize(AEAudioFormat &format);
  virtual void Reset();

  virtual unsigned int GetBitRate    ();
  virtual CodecID      GetCodecID    ();
  virtual unsigned int GetFrames     ();

  virtual int Encode (float *data, unsigned int frames);
  virtual int GetData(uint8_t **data);
  virtual double GetDelay(unsigned int bufferSize);
private:
  DllAvCodec  m_dllAvCodec;
  DllAvFormat m_dllAvFormat;
  DllAvUtil   m_dllAvUtil;

  std::string                m_CodecName;
  CodecID                   m_CodecID;
  unsigned int              m_BitRate;
  CAEPackIEC61937::PackFunc m_PackFunc;

  AEAudioFormat     m_CurrentFormat;
  AVCodecContext   *m_CodecCtx;
  CAEChannelInfo    m_Layout;
  uint8_t           m_Buffer[IEC61937_DATA_OFFSET + FF_MIN_BUFFER_SIZE];
  int               m_BufferSize;
  int               m_OutputSize;
  double            m_OutputRatio;
  double            m_SampleRateMul;

  unsigned int      m_NeededFrames;

  unsigned int BuildChannelLayout(const int64_t ffmap, CAEChannelInfo& layout);
};

