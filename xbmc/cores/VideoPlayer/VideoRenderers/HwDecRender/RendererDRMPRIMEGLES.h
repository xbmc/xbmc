/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

#include <memory>

extern "C"
{
#include <libavutil/mastering_display_metadata.h>
}

namespace Shaders
{
class YUV2RGBProgressiveShader;
}

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

class CDRMPRIMETexture;
class CVideoLayerBridgeDRMPRIME;

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
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void UnInit() override {}
  bool Flush(bool saveBuffers) override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;
  CRenderInfo GetRenderInfo() override;
  void Update() override;
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool RenderCapture(CRenderCapture* capture) override;
  bool ConfigChanged(const VideoPicture& picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

private:
  void DrawBlackBars();

  AVColorPrimaries GetSrcPrimaries(AVColorPrimaries srcPrimaries,
                                   unsigned int width,
                                   unsigned int height);

  bool m_reloadShaders;

  EShaderFormat m_shaderFormat{SHADER_NONE};

  bool m_configured = false;
  float m_clearColour{0.0f};

  bool m_fullRange;
  AVColorPrimaries m_srcPrimaries;
  bool m_toneMap = false;

  struct BUFFER
  {
    CVideoBuffer* videoBuffer = nullptr;
    std::unique_ptr<KODI::UTILS::EGL::CEGLFence> fence;
    std::unique_ptr<CDRMPRIMETexture> texture;

    AVColorPrimaries m_srcPrimaries;
    AVColorSpace m_srcColSpace;
    int m_srcBits{8};
    int m_srcTextureBits{8};
    bool m_srcFullRange;

    bool hasDisplayMetadata{false};
    AVMasteringDisplayMetadata displayMetadata;
    bool hasLightMetadata{false};
    AVContentLightMetadata lightMetadata;
  } m_buffers[NUM_BUFFERS];

  std::unique_ptr<Shaders::YUV2RGBProgressiveShader> m_progressiveShader;
};
