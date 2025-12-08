/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BaseRenderer.h"
#include "windows/RendererBase.h"

struct VideoPicture;
class CRenderCapture;

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  void Update() override;
  bool RenderCapture(int index, CRenderCapture* capture) override;

  // Player functions
  bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;
  void AddVideoPicture(const VideoPicture &picture, int index) override;
  void UnInit() override;
  bool IsConfigured() override { return m_bConfigured; }
  bool Flush(bool saveBuffers) override;
  CRenderInfo GetRenderInfo() override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  void SetBufferSize(int numBuffers) override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; }
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;

  bool WantsDoublePass() override;
  bool ConfigChanged(const VideoPicture& picture) override;

  // Debug info video
  DEBUG_INFO_VIDEO GetDebugInfo(int idx) override;

  CRenderCapture* GetRenderCapture() override;

protected:
  void PreInit();
  int NextBuffer() const;
  CRendererBase* SelectRenderer(const VideoPicture &picture);
  CRect GetScreenRect() const;

  bool m_bConfigured = false;
  std::unique_ptr<CRendererBase> m_renderer;
};
