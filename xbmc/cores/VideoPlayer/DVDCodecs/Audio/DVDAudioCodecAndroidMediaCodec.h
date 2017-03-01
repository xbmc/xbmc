#pragma once
/*
 *      Copyright (C) 2016 Christian Browet
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

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

  // required overrides
public:
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  virtual void Dispose() override;
  virtual int AddData(const DemuxPacket &packet) override;
  virtual void GetData(DVDAudioFrame &frame) override;
  virtual int GetData(uint8_t** dst) override;
  virtual void Reset() override;
  virtual AEAudioFormat GetFormat() override{ return m_format; }
  virtual const char* GetName() override { return "mediacodec"; }
  
protected:
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
  bool m_opened;
  int m_samplerate;
  int m_channels;
  uint8_t* m_buffer;
  int m_bufferSize;
  int m_bufferUsed;
  AEAudioFormat m_format;
  double m_currentPts;

  std::shared_ptr<CJNIMediaCodec> m_codec;
  CJNIMediaCrypto *m_crypto;
};
