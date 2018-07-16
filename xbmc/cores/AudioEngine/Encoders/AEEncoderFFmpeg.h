/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Interfaces/AEEncoder.h"

extern "C" {
#include "libswresample/swresample.h"
}

/* ffmpeg re-defines this, so undef it to squash the warning */
#undef restrict

class CAEEncoderFFmpeg: public IAEEncoder
{
public:
  CAEEncoderFFmpeg();
  ~CAEEncoderFFmpeg() override;

  bool IsCompatible(const AEAudioFormat& format) override;
  bool Initialize(AEAudioFormat &format, bool allow_planar_input = false) override;
  void Reset() override;

  unsigned int GetBitRate() override;
  AVCodecID GetCodecID() override;
  unsigned int GetFrames() override;

  int Encode(uint8_t *in, int in_size, uint8_t *out, int out_size) override;
  int GetData(uint8_t **data) override;
  double GetDelay(unsigned int bufferSize) override;
private:
  unsigned int BuildChannelLayout(const int64_t ffmap, CAEChannelInfo& layout);

  std::string m_CodecName;
  AVCodecID m_CodecID;
  unsigned int m_BitRate = 0;
  AEAudioFormat m_CurrentFormat;
  AVCodecContext *m_CodecCtx;
  SwrContext *m_SwrCtx;
  CAEChannelInfo m_Layout;
  AVPacket m_Pkt;
  uint8_t m_Buffer[8 + AV_INPUT_BUFFER_MIN_SIZE];
  int m_BufferSize = 0;
  int m_OutputSize = 0;
  double m_OutputRatio = 0.0;
  double m_SampleRateMul = 0.0;
  unsigned int  m_NeededFrames = 0;
  bool m_NeedConversion = false;
};

