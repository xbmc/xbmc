#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DVDAudioCodec.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

class CProcessInfo;

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg(CProcessInfo &processInfo);
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints,
                    CDVDCodecOptions &options) override;
  virtual void Dispose() override;
  virtual int AddData(const DemuxPacket &packet) override;
  virtual void GetData(DVDAudioFrame &frame) override;
  virtual int GetData(uint8_t** dst) override;
  virtual void Reset() override;
  virtual AEAudioFormat GetFormat() override{ return m_format; }
  virtual const char* GetName() override { return "FFmpeg"; }
  virtual enum AVMatrixEncoding GetMatrixEncoding() override;
  virtual enum AVAudioServiceType GetAudioServiceType() override;
  virtual int GetProfile() override;

protected:
  enum AEDataFormat GetDataFormat();
  int GetSampleRate();
  int GetChannels();
  CAEChannelInfo GetChannelMap();
  int GetBitRate() override;
  void BuildChannelMap();
  void ConvertToFloat();

  AEAudioFormat m_format;
  AVCodecContext* m_pCodecContext;
  enum AVSampleFormat m_iSampleFormat;
  CAEChannelInfo m_channelLayout;
  enum AVMatrixEncoding m_matrixEncoding;
  AVFrame* m_pFrame;
  bool m_eof;
  int m_channels;
  uint64_t m_layout;
};

