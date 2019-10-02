/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef HAS_GL
#include <OpenGL/gl.h>
#else
#include <OpenGLES/ES2/gl.h>
#endif

#include "DVDVideoCodecFFmpeg.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"
#include <CoreVideo/CVPixelBuffer.h>

class CProcessInfo;

namespace VTB
{
class CVideoBufferVTB;
class CVideoBufferPoolVTB;

class CVideoBufferVTB: public CVideoBuffer
{
public:
  CVideoBufferVTB(IVideoBufferPool &pool, int id);
  ~CVideoBufferVTB() override;
  void SetRef(AVFrame *frame);
  void Unref();
  CVPixelBufferRef GetPB();

  GLuint m_fence = 0;
protected:
  CVPixelBufferRef m_pbRef = nullptr;
  AVFrame *m_pFrame;
};

class CDecoder: public IHardwareDecoder
{
public:
  CDecoder(CProcessInfo& processInfo);
  ~CDecoder() override;
  static IHardwareDecoder* Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt);
  static bool Register();
  bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) override;
  CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) override;
  bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) override;
  CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) override;
  const std::string Name() override { return "vtb"; }
  unsigned GetAllowedReferences() override;

  void Close();

protected:
  unsigned m_renderbuffers_count;
  AVCodecContext *m_avctx;
  CProcessInfo& m_processInfo;
  CVideoBufferVTB *m_renderBuffer = nullptr;
  std::shared_ptr<CVideoBufferPoolVTB> m_videoBufferPool;
};

}
