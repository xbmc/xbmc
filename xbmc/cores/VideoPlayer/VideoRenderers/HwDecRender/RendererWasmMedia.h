/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

/*!
 * \brief Places the <video> of a media backend, which shows through the transparent GUI
 * canvas.
 */
class CRendererWasmMedia final : public CBaseRenderer
{
public:
  ~CRendererWasmMedia() override;

  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static bool Register();

  void AddVideoPicture(const VideoPicture& picture, int index) override {}
  void ReleaseBuffer(int idx) override {}
  CRenderInfo GetRenderInfo() override;
  bool IsGuiLayer() override { return false; }
  bool IsConfigured() override { return m_configured; }
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool ConfigChanged(const VideoPicture& picture) override;
  bool Supports(ERENDERFEATURE feature) const override;
  bool Supports(ESCALINGMETHOD method) const override { return false; }
  bool SupportsMultiPassRendering() override { return false; }
  void UnInit() override;
  void Update() override {}
  void RenderUpdate(
      int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;

protected:
  void ManageRenderArea() override;

private:
  CRect m_placedRect;
  bool m_configured{false};
};
