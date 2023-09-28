/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"

class CRendererStarfish : public CBaseRenderer
{
public:
  CRendererStarfish();
  ~CRendererStarfish() override;

  // Registration
  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static bool Register();

  // Player functions
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void ReleaseBuffer(int idx) override;

  // Feature support
  CRenderInfo GetRenderInfo() override;

  bool IsGuiLayer() override;
  bool IsConfigured() override;
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool SupportsMultiPassRendering() override;
  void UnInit() override;
  void Update() override;
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool RenderCapture(CRenderCapture* capture) override;
  bool ConfigChanged(const VideoPicture& picture) override;

protected:
  // hooks for hw dec renderer
  void ManageRenderArea() override;

private:
  CRect m_exportedSourceRect;
  CRect m_exportedDestRect;
  bool m_configured{false};
  long m_acbId{0};
};
