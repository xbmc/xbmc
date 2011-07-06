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
#include "AEPackIEC61937.h"

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict
#include "DllAvCodec.h"
#include "DllAvFormat.h"

class CAEEncoderFFmpeg: public IAEEncoder
{
public:
  CAEEncoderFFmpeg();
  virtual ~CAEEncoderFFmpeg();

  virtual bool IsCompatible(const AEAudioFormat format);
  virtual bool Initialize(AEAudioFormat &format);
  virtual void Reset();

  virtual unsigned int GetBitRate    ();
  virtual CodecID      GetCodecID    ();
  virtual unsigned int GetFrames     ();

  virtual int Encode (float *data, unsigned int frames);
  virtual int GetData(uint8_t **data);
  virtual float GetDelay(unsigned int bufferSize);
private:
  DllAvCodec  m_dllAvCodec;
  DllAvFormat m_dllAvFormat;
  DllAvUtil   m_dllAvUtil;

  CStdString                m_CodecName;
  CodecID                   m_CodecID;
  unsigned int              m_BitRate;
  CAEPackIEC61937::PackFunc m_PackFunc;

  AEAudioFormat     m_CurrentFormat;
  AVCodecContext   *m_CodecCtx;
  enum AEChannel    m_Layout[AE_CH_MAX+1];
  uint8_t           m_Buffer[FF_MIN_BUFFER_SIZE];
  int               m_BufferSize;
  int               m_OutputSize;
  float             m_OutputRatio;

  unsigned int      m_NeededFrames;

  unsigned int BuildChannelLayout(const int64_t ffmap, AEChLayout layout);
};

