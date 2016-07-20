/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <memory>
#include <queue>
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "libavcodec/avcodec.h"
#include "MMALCodec.h"

class CMMALRenderer;
class CMMALPool;
struct MMAL_BUFFER_HEADER_T;
class CGPUMEM;

namespace MMAL {

class CDecoder;
class CGPUPool;

// a mmal video frame
class CMMALYUVBuffer : public CMMALBuffer
{
public:
  CMMALYUVBuffer(std::shared_ptr<CMMALPool> pool, uint32_t mmal_encoding, uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, uint32_t size);
  virtual ~CMMALYUVBuffer();

  CGPUMEM *gmem;
private:
  std::shared_ptr<CMMALPool> m_pool;
};

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
{
public:
  CDecoder();
  virtual ~CDecoder();
  virtual bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat, unsigned int surfaces);
  virtual int Decode(AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual int Check(AVCodecContext* avctx);
  virtual void Close();
  virtual const std::string Name() { return "mmal"; }
  virtual unsigned GetAllowedReferences();
  virtual long Release();

  static void FFReleaseBuffer(void *opaque, uint8_t *data);
  static int FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags);

protected:
  AVCodecContext *m_avctx;
  unsigned int m_shared;
  CCriticalSection m_section;
  std::shared_ptr<CMMALPool> m_pool;
  enum AVPixelFormat m_fmt;
};

};
