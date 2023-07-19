/*
 *  Copyright (C) 2016 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDAudioCodec.h"
#include "DVDStreamInfo.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

#include <memory>
#include <queue>
#include <vector>

class CJNIMediaCodec;
class CJNIMediaCrypto;
class CJNIMediaFormat;
class CJNIByteBuffer;
class CProcessInfo;

struct DemuxPacket;

class CDVDAudioCodecAndroidMediaCodec : public CDVDAudioCodec
{
public:
  CDVDAudioCodecAndroidMediaCodec(CProcessInfo &processInfo);
  ~CDVDAudioCodecAndroidMediaCodec() override;

  // registration
  static std::unique_ptr<CDVDAudioCodec> Create(CProcessInfo& processInfo);
  static bool Register();

  // required overrides
public:
  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  void Dispose() override;
  bool AddData(const DemuxPacket &packet) override;
  void GetData(DVDAudioFrame &frame) override;
  void Reset() override;
  AEAudioFormat GetFormat() override;
  std::string GetName() override;

protected:
  int GetData(uint8_t** dst);
  int GetChannels() { return m_channels; }
  int GetEncodedChannels() { return m_channels; }
  CAEChannelInfo GetChannelMap();
  int GetSampleRate() { return m_samplerate; }
  int GetEncodedSampleRate() { return m_samplerate; }
  enum AEDataFormat GetDataFormat() { return AE_FMT_S16NE; }

  bool ConfigureMediaCodec(void);
  void ConfigureOutputFormat(CJNIMediaFormat* mediaformat);

  CDVDStreamInfo m_hints;
  std::string m_mime;
  std::string m_codecname;
  std::string m_formatname;
  bool m_opened = false, m_codecIsFed = false;
  int m_samplerate = 0;
  int m_channels = 0;
  uint8_t* m_buffer;
  int m_bufferSize = 0;
  int m_bufferUsed = 0;
  AEAudioFormat m_format;
  double m_currentPts = DVD_NOPTS_VALUE;

  std::shared_ptr<CJNIMediaCodec> m_codec;
  CJNIMediaCrypto* m_crypto = nullptr;
  std::shared_ptr<CDVDAudioCodec> m_decryptCodec;
};
