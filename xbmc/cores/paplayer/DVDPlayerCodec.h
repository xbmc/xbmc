#ifndef DVDPLAYER_CODEC_H_
#define DVDPLAYER_CODEC_H_

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "cores/dvdplayer/DVDDemuxers/DVDDemux.h"
#include "cores/dvdplayer/DVDCodecs/Audio/DVDAudioCodec.h"
#include "cores/dvdplayer/DVDInputStreams/DVDInputStream.h"

class DVDPlayerCodec : public ICodec
{
public:
  DVDPlayerCodec();
  virtual ~DVDPlayerCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual int64_t Seek(int64_t iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  virtual bool CanSeek();
  virtual CAEChannelInfo GetChannelInfo() {return m_ChannelInfo;}

  void SetContentType(const CStdString &strContent);

private:
  CDVDDemux* m_pDemuxer;
  CDVDInputStream* m_pInputStream;
  CDVDAudioCodec* m_pAudioCodec;

  CStdString m_strContentType;

  int m_nAudioStream;

  int m_audioPos;
  DemuxPacket* m_pPacket ;

  BYTE *m_decoded;
  int  m_nDecodedLen;

  CAEChannelInfo m_ChannelInfo;
};

#endif
