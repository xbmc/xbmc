/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <queue>
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "libavcodec/avcodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/MMALRenderer.h"

struct MMAL_BUFFER_HEADER_T;
class CGPUMEM;

namespace MMAL {

class CDecoder;

// a mmal video frame
class CMMALYUVBuffer : public CMMALBuffer
{
public:
  CMMALYUVBuffer(int id);
  virtual ~CMMALYUVBuffer();
  uint8_t* GetMemPtr() override;
  virtual void GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES]) override;
  virtual void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) override;
  virtual void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES]) override;
  virtual void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES], const int (&planeOffsets)[YuvImage::MAX_PLANES]) override;
  CGPUMEM *Allocate(int size, void *opaque);
  CGPUMEM *GetMem() { return m_gmem; }
protected:
  CGPUMEM *m_gmem = nullptr;
};

class CDecoder
  : public IHardwareDecoder
{
public:
  CDecoder(CProcessInfo& processInfo, CDVDStreamInfo &hints);
  virtual ~CDecoder();
  virtual bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  virtual CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  virtual bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  virtual CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  virtual const std::string Name() override { return "mmal"; }
  virtual unsigned GetAllowedReferences() override;
  virtual long Release() override;

  static void AlignedSize(AVCodecContext *avctx, int &width, int &height);
  static void FFReleaseBuffer(void *opaque, uint8_t *data);
  static int FFGetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags);
  static IHardwareDecoder* Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt);
  static void Register();

protected:
  AVCodecContext *m_avctx;
  CProcessInfo &m_processInfo;
  CCriticalSection m_section;
  std::shared_ptr<CMMALPool> m_pool;
  enum AVPixelFormat m_fmt;
  CDVDStreamInfo m_hints;
  CMMALYUVBuffer *m_renderBuffer = nullptr;
};

};
