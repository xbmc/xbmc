/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "DVDAudioCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/channel_layout.h"
#include "libswresample/swresample.h"
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
  std::string GetName() override { return m_codecName; };
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
  enum AVSampleFormat m_iSampleFormat;
  CAEChannelInfo m_channelLayout;
  enum AVMatrixEncoding m_matrixEncoding = AV_MATRIX_ENCODING_NONE;
  AVFrame* m_pFrame;
  bool m_eof;
  int m_channels;
  uint64_t m_layout;
  std::string m_codecName;
};

