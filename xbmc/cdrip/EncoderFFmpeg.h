#ifndef _ENCODERFFMPEG_H
#define _ENCODERFFMPEG_H

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

#include "Encoder.h"
#include "DllAvFormat.h"
#include "DllAvCodec.h"
#include "DllAvUtil.h"

class CEncoderFFmpeg : public CEncoder
{
public:
  CEncoderFFmpeg();
  virtual ~CEncoderFFmpeg() {}
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, uint8_t* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

private:
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  DllAvFormat m_dllAvFormat;

  AVFormatContext  *m_Format;
  AVCodecContext   *m_CodecCtx;
  AVStream         *m_Stream;
  AVPacket          m_Pkt;
  unsigned char     m_BCBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  static int        MuxerReadPacket(void *opaque, uint8_t *buf, int buf_size);
  void              SetTag(const CStdString tag, const CStdString value);


  unsigned int      m_NeededFrames;
  unsigned int      m_NeededBytes;
  uint8_t          *m_Buffer;
  unsigned int      m_BufferSize;

  bool WriteFrame();
};

#endif // _ENCODERFFMPEG_H
