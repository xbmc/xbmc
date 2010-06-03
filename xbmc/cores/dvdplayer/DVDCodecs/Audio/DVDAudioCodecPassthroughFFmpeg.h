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

#include <list>
#include "system.h"
#include "Codecs/DllAvFormat.h"
#include "Codecs/DllAvCodec.h"

#include "../DVDFactoryCodec.h"
#include "DVDAudioCodec.h"

class IDVDAudioEncoder;

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
  virtual int GetBufferSize();
private:
  DllAvFormat m_dllAvFormat;
  DllAvUtil   m_dllAvUtil;
  DllAvCodec  m_dllAvCodec;

  typedef struct
  {
    int      size;
    uint8_t *data;
  } DataPacket;

  typedef struct
  {
    AVFormatContext       *m_pFormat;
    AVStream              *m_pStream;
    std::list<DataPacket*> m_OutputBuffer;
    unsigned int           m_OutputSize;
    bool                   m_WroteHeader;
    unsigned char          m_BCBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    unsigned int           m_Consumed;
    unsigned int           m_BufferSize;
    uint8_t               *m_Buffer;
  } Muxer;

  Muxer      m_SPDIF, m_ADTS;
  bool       SetupMuxer(CDVDStreamInfo &hints, CStdString muxerName, Muxer &muxer);
  static int MuxerReadPacket(void *opaque, uint8_t *buf, int buf_size);
  void       WriteFrame(Muxer &muxer, uint8_t *pData, int iSize);
  int        GetMuxerData(Muxer &muxer, uint8_t** dst);
  void       ResetMuxer(Muxer &muxer);
  void       DisposeMuxer(Muxer &muxer);

  bool m_bSupportsAC3Out;
  bool m_bSupportsDTSOut;
  bool m_bSupportsAACOut;
  bool m_bSupportsMP1Out;
  bool m_bSupportsMP2Out;
  bool m_bSupportsMP3Out;

  CDVDAudioCodec   *m_Codec;
  IDVDAudioEncoder *m_Encoder;
  bool              m_InitEncoder;
  unsigned int      m_EncPacketSize;
  BYTE             *m_DecodeBuffer;
  unsigned int      m_DecodeSize;
  bool SupportsFormat(CDVDStreamInfo &hints);
  bool SetupEncoder  (CDVDStreamInfo &hints);

  uint8_t      m_NeededBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  unsigned int m_NeededUsed;
  unsigned int m_Needed;
  bool         m_LostSync;
  int          m_SampleRate;

  unsigned int (CDVDAudioCodecPassthroughFFmpeg::*m_pSyncFrame)(BYTE* pData, unsigned int iSize, unsigned int *fSize);
  unsigned int SyncAC3(BYTE* pData, unsigned int iSize, unsigned int *fSize);
  unsigned int SyncDTS(BYTE* pData, unsigned int iSize, unsigned int *fSize);
  unsigned int SyncAAC(BYTE* pData, unsigned int iSize, unsigned int *fSize);
};

