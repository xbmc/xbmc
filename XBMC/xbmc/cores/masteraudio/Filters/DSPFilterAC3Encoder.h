/*
 *      Copyright (C) 2009 Team XBMC
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

#ifndef __DSPFILTERAC3ENCODER_H__
#define __DSPFILTERAC3ENCODER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DSPFilterLPCM.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/ac3.h"

// The AC3 spec defines a frame as a header + 6 coded audio blocks, each containing 256 input samples
#define AC3_SAMPLES_PER_FRAME AC3_FRAME_SIZE // Audio samples per AC3 frame. 

// Base class for LPCM DSPFilters
class CDSPFilterAC3Encoder : public CDSPFilterLPCM
{
public:
  CDSPFilterAC3Encoder(unsigned int inputBusses = 1, unsigned int outputBusses = 1);
  virtual ~CDSPFilterAC3Encoder();

  // IAudioSink
  virtual MA_RESULT TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);

  // IAudioSource
  virtual MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus = 0);
  virtual MA_RESULT Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus = 0);

protected:

private:
  struct EncoderAttributes {
    AVCodecContext     *m_CodecContext;   // The codec context
    unsigned int        m_SampleRate;      // The samplerate
    ma_audio_container *m_pInputContainer; // The input audio container
  };

  EncoderAttributes *m_pEncoderAttributes;
  DllAvCodec m_AvCodec;
  DllAvUtil  m_AvUtil;

  // Internal init function
  MA_RESULT InitCodec(CStreamDescriptor *pDesc, unsigned int bus);
};

#endif // __DSPFILTERAC3ENCODER_H__
