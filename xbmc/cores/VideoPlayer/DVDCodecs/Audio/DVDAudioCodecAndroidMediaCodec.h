/*
 *      Copyright (C) 2016 Christian Browet
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <queue>
#include <vector>
#include <memory>

#include "DVDAudioCodec.h"
#include "DVDStreamInfo.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"

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
  virtual ~CDVDAudioCodecAndroidMediaCodec();

  // registration
  static CDVDAudioCodec* Create(CProcessInfo &processInfo);
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
  bool m_opened, m_codecIsFed;
  int m_samplerate;
  int m_channels;
  uint8_t* m_buffer;
  int m_bufferSize;
  int m_bufferUsed;
  AEAudioFormat m_format;
  double m_currentPts;

  std::shared_ptr<CJNIMediaCodec> m_codec;
  CJNIMediaCrypto *m_crypto;
  std::shared_ptr<CDVDAudioCodec> m_decryptCodec;
};
