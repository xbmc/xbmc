/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VaapiEGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

#include <memory>

namespace VAAPI
{
class IVaapiWinSystem;
}

class CRendererVAAPIGL : public CLinuxRendererGL
{
public:
  CRendererVAAPIGL();
  ~CRendererVAAPIGL() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static void Register(VAAPI::IVaapiWinSystem *winSystem, VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &deepColor);

  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;

  // Player functions
  bool ConfigChanged(const VideoPicture &picture) override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;
  bool Flush(bool saveBuffers) override;

  // Feature support
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

protected:
  bool LoadShadersHook() override;
  bool RenderHook(int idx) override;
  void AfterRenderHook(int idx) override;

  // textures
  bool UploadTexture(int index) override;
  void DeleteTexture(int index) override;
  bool CreateTexture(int index) override;

  EShaderFormat GetShaderFormat() override;

  bool m_isVAAPIBuffer = true;
  std::unique_ptr<VAAPI::CVaapiTexture> m_vaapiTextures[NUM_BUFFERS];
  GLsync m_fences[NUM_BUFFERS];
  static VAAPI::IVaapiWinSystem *m_pWinSystem;
};
