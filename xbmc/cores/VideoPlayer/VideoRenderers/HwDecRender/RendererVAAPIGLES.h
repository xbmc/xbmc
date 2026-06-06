/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VaapiEGL.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

#include <array>
#include <memory>

namespace KODI
{
namespace UTILS
{
namespace EGL
{
class CEGLFence;
}
}
}

namespace VAAPI
{
class IVaapiWinSystem;
}

class CRendererVAAPIGLES : public CLinuxRendererGLES
{
public:
  CRendererVAAPIGLES();
  ~CRendererVAAPIGLES() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static void Register(VAAPI::IVaapiWinSystem *winSystem, VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &deepColor);

  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;

  // Player functions
  bool ConfigChanged(const VideoPicture &picture) override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;

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
  bool m_nv12Allocated[NUM_BUFFERS]{};
  // VA fourcc of the surfaces being received (NV12 / P010 / P012 / P016 /
  // YUY2 / Y210 / Y212 / Y216). Captured at Configure and read by
  // GetShaderFormat to pick the matching sampling path.
  std::int32_t m_vaapiFourcc{};
  std::unique_ptr<VAAPI::CVaapiTexture> m_vaapiTextures[NUM_BUFFERS];
  std::array<std::unique_ptr<KODI::UTILS::EGL::CEGLFence>, NUM_BUFFERS> m_fences;
  static VAAPI::IVaapiWinSystem *m_pWinSystem;
};
