/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMPRIMEEGL.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

#include <memory>

namespace KODI
{
namespace UTILS
{
namespace EGL
{
class CEGLFence;
}
} // namespace UTILS
} // namespace KODI

namespace Shaders
{
namespace GLES
{
class BaseYUV2RGBGLSLShader;
}
} // namespace Shaders

class CRendererDRMPRIMEGLES : public CBaseRenderer
{
public:
  CRendererDRMPRIMEGLES() = default;
  ~CRendererDRMPRIMEGLES() override;

  // Registration
  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static void Register();

  // Player functions
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_configured; }
  bool IsGuiLayer() override;
  bool HasVideoPlane() override { return false; }
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void UnInit() override;
  bool Flush(bool saveBuffers) override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;
  CRenderInfo GetRenderInfo() override;
  void Update() override;
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool RenderCapture(int index, CRenderCapture* capture) override;
  bool ConfigChanged(const VideoPicture& picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

private:
  void DrawBlackBars();
  void Render(unsigned int flags, int index);

  bool m_configured = false;
  bool m_passthroughHDR{false};
  bool m_hdrFboActive{false};

  // Limited-range path: per-plane EGL import + standard YUV2RGB shader.
  // Set at Configure time when the user has limited-range output enabled
  // AND the buffer's source fourcc can be imported by CDRMPRIMETextureYUV
  // AND the shader compiles. If null, Render() falls through to the OES
  // path (which always outputs full-range RGB).
  std::unique_ptr<Shaders::GLES::BaseYUV2RGBGLSLShader> m_yuvShader;

  struct BUFFER
  {
    CVideoBuffer* videoBuffer = nullptr;
    std::unique_ptr<KODI::UTILS::EGL::CEGLFence> fence;
    CDRMPRIMETexture texture;
    CDRMPRIMETextureYUV yuvTexture;
  } m_buffers[NUM_BUFFERS];
};
