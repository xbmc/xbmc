/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

#include <CoreVideo/CVOpenGLESTextureCache.h>

class CRendererVTB : public CLinuxRendererGLES
{
public:
  CRendererVTB();
  ~CRendererVTB() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  // Player functions
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;

protected:
  // hooks for hw dec renderer
  bool LoadShadersHook() override;
  void AfterRenderHook(int idx) override;
  EShaderFormat GetShaderFormat() override;

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  CVOpenGLESTextureCacheRef m_textureCache = nullptr;
  struct CRenderBuffer
  {
    CVOpenGLESTextureRef m_textureY;
    CVOpenGLESTextureRef m_textureUV;
    CVBufferRef m_videoBuffer;
    GLsync m_fence;
  };
  CRenderBuffer m_vtbBuffers[NUM_BUFFERS];
  CVEAGLContext m_glContext;
};

