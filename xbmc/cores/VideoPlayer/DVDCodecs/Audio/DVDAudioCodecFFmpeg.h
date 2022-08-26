/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDAudioCodec.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libavutil/downmix_info.h>
}

class CProcessInfo;

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  explicit CDVDAudioCodecFFmpeg(CProcessInfo &processInfo);
  ~CDVDAudioCodecFFmpeg() override;
  bool Open(CDVDStreamInfo &hints,
                    CDVDCodecOptions &options) override;
  void Dispose() override;
  bool AddData(const DemuxPacket &packet) override;
  void GetData(DVDAudioFrame &frame) override;
  void Reset() override;
  AEAudioFormat GetFormat() override { return m_format; }
  std::string GetName() override { return m_codecName; }
  enum AVMatrixEncoding GetMatrixEncoding() override;
  enum AVAudioServiceType GetAudioServiceType() override;
  int GetProfile() override;

protected:
  int GetData(uint8_t** dst);
  enum AEDataFormat GetDataFormat();
  int GetSampleRate();
  int GetChannels();
  CAEChannelInfo GetChannelMap();
  int GetBitRate() override;
  void BuildChannelMap();

  AEAudioFormat m_format;
  AVCodecContext* m_pCodecContext;
  enum AVSampleFormat m_iSampleFormat = AV_SAMPLE_FMT_NONE;
  CAEChannelInfo m_channelLayout;
  enum AVMatrixEncoding m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  AVFrame* m_pFrame;
  AVDownmixInfo m_downmixInfo;
  bool m_hasDownmix = false;
  bool m_eof;
  int m_channels;
  uint64_t m_layout;
  std::string m_codecName;
  uint64_t m_hint_layout;
};

