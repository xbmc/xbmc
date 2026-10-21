/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

#include <atomic>
#include <cstdint>

// Draws CVideoBufferWebCodecs pictures: the VideoFrame the buffer names is
// imported into this renderer's texture on the browser main thread, in the
// render thread's own GL command order, and drawn with the GUI's RGBA shader.
// The browser does the colour conversion and scaling; the base class keeps
// geometry, aspect, zoom, rotation and the framebuffer path.
class CRendererWebCodecs : public CLinuxRendererGLES
{
public:
  CRendererWebCodecs();
  ~CRendererWebCodecs() override;

  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static bool Register();

  bool ConfigChanged(const VideoPicture& picture) override;
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  CRenderInfo GetRenderInfo() override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

protected:
  bool LoadShadersHook() override;
  EShaderFormat GetShaderFormat() override;
  bool RenderHook(int index) override;
  bool CreateTexture(int index) override;
  void DeleteTexture(int index) override;
  bool UploadTexture(int index) override;

private:
  GLenum TextureFilter() const;
  void ApplyTextureFilter();
  void UploadPendingFrames();

  static_assert(NUM_BUFFERS <= 32, "m_pendingUploads holds one bit per render buffer");
  // Bits set by AddVideoPicture on the VideoPlayer thread, taken by the render thread.
  std::atomic<uint32_t> m_pendingUploads{0};
  ESCALINGMETHOD m_appliedScalingMethod{VS_SCALINGMETHOD_MAX};
  unsigned int m_alpha{255};
};
