#ifndef _ENCODERFFMPEG_H
#define _ENCODERFFMPEG_H

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

#include "IEncoder.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}

class CEncoderFFmpeg : public IEncoder
{
public:
  CEncoderFFmpeg();
  virtual ~CEncoderFFmpeg() {}

  bool Init(audioenc_callbacks &callbacks);
  int Encode(int nNumBytesRead, uint8_t *pbtStream);
  bool Close();
private:

  AVFormatContext  *m_Format;
  AVCodecContext   *m_CodecCtx;
  SwrContext       *m_SwrCtx;
  AVStream         *m_Stream;
  AVPacket          m_Pkt;
  AVSampleFormat    m_InFormat;
  AVSampleFormat    m_OutFormat;
  /* From libavformat/avio.h:
   * The buffer size is very important for performance.
   * For protocols with fixed blocksize it should be set to this
   * blocksize.
   * For others a typical size is a cache page, e.g. 4kb.
   */
  unsigned char     m_BCBuffer[4096];
  static int        avio_write_callback(void *opaque, uint8_t *buf, int buf_size);
  static int64_t    avio_seek_callback(void *opaque, int64_t offset, int whence);
  void              SetTag(const std::string &tag, const std::string &value);

  unsigned int      m_NeededFrames;
  unsigned int      m_NeededBytes;
  uint8_t          *m_Buffer;
  unsigned int      m_BufferSize;
  AVFrame          *m_BufferFrame;
  uint8_t          *m_ResampledBuffer;
  unsigned int      m_ResampledBufferSize;
  AVFrame          *m_ResampledFrame;
  bool              m_NeedConversion;

  audioenc_callbacks m_callbacks;

  bool WriteFrame();
};

#endif // _ENCODERFFMPEG_H
