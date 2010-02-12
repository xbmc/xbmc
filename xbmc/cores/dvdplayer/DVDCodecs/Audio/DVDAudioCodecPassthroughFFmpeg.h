#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"

#include "DVDAudioCodec.h"

class CDVDAudioCodecPassthroughFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPassthroughFFmpeg();
  virtual ~CDVDAudioCodecPassthroughFFmpeg();

  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual enum PCMChannels *GetChannelMap() { static enum PCMChannels map[2] = {PCM_FRONT_LEFT, PCM_FRONT_RIGHT}; return map; }
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPassthrough() { return true; }
  virtual const char* GetName()  { return "PassthroughFFmpeg"; }

private:
  int (CDVDAudioCodecPassthroughFFmpeg::*m_pSyncFrame)(BYTE* pData, int iSize, int *fSize);
  int SyncAC3(BYTE* pData, int iSize, int *fSize);
  int SyncDTS(BYTE* pData, int iSize, int *fSize);

  DllAvFormat      m_dllAvFormat;
  DllAvUtil        m_dllAvUtil;
  DllAvCodec       m_dllAvCodec;

  AVFormatContext *m_pFormat;
  AVStream        *m_pStream;

  unsigned char    m_bcBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  BYTE            *m_OutputBuffer;
  int              m_OutputSize;
  bool             m_lostSync;

  static int _BCReadPacket(void *opaque, uint8_t *buf, int buf_size) { return ((CDVDAudioCodecPassthroughFFmpeg*)opaque)->BCReadPacket(buf, buf_size); }
  int BCReadPacket(uint8_t *buf, int buf_size);
};

