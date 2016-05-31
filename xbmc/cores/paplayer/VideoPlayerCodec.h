#ifndef VideoPlayer_CODEC_H_
#define VideoPlayer_CODEC_H_

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

#include "ICodec.h"

#include "cores/VideoPlayer/DVDDemuxers/DVDDemux.h"
#include "cores/VideoPlayer/DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"

namespace ActiveAE
{
  class IAEResample;
};

class VideoPlayerCodec : public ICodec
{
public:
  VideoPlayerCodec();
  virtual ~VideoPlayerCodec();

  virtual bool Init(const CFileItem &file, unsigned int filecache) override;
  virtual void DeInit();
  virtual bool Seek(int64_t iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual int ReadRaw(uint8_t **pBuffer, int *bufferSize);
  virtual bool CanInit();
  virtual bool CanSeek();
  virtual CAEChannelInfo GetChannelInfo() {return m_srcFormat.m_channelLayout;}

  AEAudioFormat GetFormat();
  void SetContentType(const std::string &strContent);

  bool NeedConvert(AEDataFormat fmt);

private:
  CDVDDemux* m_pDemuxer;
  CDVDInputStream* m_pInputStream;
  CDVDAudioCodec* m_pAudioCodec;

  std::string m_strContentType;
  std::string m_strFileName;
  int m_nAudioStream;
  int m_audioPos;
  DemuxPacket* m_pPacket;
  int  m_nDecodedLen;

  bool m_bInited;
  bool m_bCanSeek;

  ActiveAE::IAEResample *m_pResampler;
  uint8_t *m_audioPlanes[8];
  int m_planes;
  bool m_needConvert;
  AEAudioFormat m_srcFormat;
  int m_channels;

  std::unique_ptr<CProcessInfo> m_processInfo;
};

#endif
