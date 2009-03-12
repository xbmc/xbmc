/*
 *      Copyright (C) 2009 phi2039
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

#ifndef __AC3_ENCODER_H__
#define __AC3_ENCODER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MA_DllAvCodec.h"
#include "../MasterAudioCore.h"

class CAC3Encoder : public IDSPFilter
{
public:
  CAC3Encoder();
  virtual ~CAC3Encoder();

  bool OpenCodec(unsigned int inputChannels, unsigned int inputSamplesPerSec, unsigned int inputBitsPerSample);
  void CloseCodec();

  // IDSPFilter Methods
  void Close();

  // IAudioSink Methods
  MA_RESULT TestInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetInputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetInputProperties(audio_data_transfer_props* pProps);
  MA_RESULT AddSlice(audio_slice* pSlice);
  float GetMaxLatency();
  void Flush();

  // IAudioSource Methods
  MA_RESULT TestOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT SetOutputFormat(CStreamDescriptor* pDesc);
  MA_RESULT GetOutputProperties(audio_data_transfer_props* pProps);
  MA_RESULT GetSlice(audio_slice** ppSlice);

private:
  LibAvCodec m_avLib; // LibAvCodec Wrapper Object
  AVCodecContext* m_pCodecContext; // Encoding Session Information
  bool m_CodecIsOpen;
  size_t m_FrameDataLen; // Bytes per input frame
  audio_slice* m_pInputSlice; // Storage for the most recently-provided slice
  size_t m_InputBufferPos; // Offset within the input buffer
  int m_Channels; // Channels in the current stream
};

#endif // __AC3_ENCODER_H__