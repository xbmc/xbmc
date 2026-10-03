/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "threads/CriticalSection.h"

// Presents the VideoToolbox P010 surface without passing it through an 8-bit
// OpenGL ES framebuffer. Kodi's GUI is composited above the system video layer.
class CRendererVTBDisplayLayer : public CBaseRenderer
{
public:
  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static bool Register();

  ~CRendererVTBDisplayLayer() override;

  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_configured; }
  bool ConfigChanged(const VideoPicture& picture) override;
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void ReleaseBuffer(int index) override;
  void UnInit() override;
  bool Flush(bool saveBuffers) override;
  void Update() override;
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool IsGuiLayer() override { return false; }
  CRenderInfo GetRenderInfo() override;
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ESCALINGMETHOD method) const override { return false; }

private:
  CVideoBuffer* m_buffers[NUM_BUFFERS]{};
  StreamHdrType m_hdrType{};
  bool m_configured{false};
  int m_lastIndex{-1};
  CRect m_lastRect;
};
