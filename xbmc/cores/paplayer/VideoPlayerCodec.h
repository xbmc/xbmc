/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ICodec.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"

namespace ActiveAE
{
  class IAEResample;
};

class VideoPlayerCodec : public ICodec
{
public:
  VideoPlayerCodec();
  ~VideoPlayerCodec() override;

  bool Init(const CFileItem &file, unsigned int filecache) override;
  bool Seek(int64_t iSeekTime) override;
  int ReadPCM(uint8_t* pBuffer, size_t size, size_t* actualsize) override;
  int ReadRaw(uint8_t **pBuffer, int *bufferSize) override;
  bool CanInit() override;
  bool CanSeek() override;

  void DeInit();
  AEAudioFormat GetFormat();
  void SetContentType(const std::string &strContent);

  bool NeedConvert(AEDataFormat fmt);
  void SetPassthroughStreamType(CAEStreamInfo::DataType streamType);

private:
  CAEStreamInfo::DataType GetPassthroughStreamType(AVCodecID codecId, int samplerate, int profile);

  CDVDDemux* m_pDemuxer;
  std::shared_ptr<CDVDInputStream> m_pInputStream;
  std::unique_ptr<CDVDAudioCodec> m_pAudioCodec;

  std::string m_strContentType;
  std::string m_strFileName;
  int m_nAudioStream;
  size_t m_nDecodedLen;

  bool m_bInited;
  bool m_bCanSeek;

  ActiveAE::IAEResample *m_pResampler;
  DVDAudioFrame m_audioFrame;
  int m_planes;
  bool m_needConvert;
  AEAudioFormat m_srcFormat;
  int m_channels;

  std::unique_ptr<CProcessInfo> m_processInfo;
};

