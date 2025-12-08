/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

class CMediaCodecVideoBuffer;

class CRendererMediaCodecSurface : public CBaseRenderer
{
public:
  CRendererMediaCodecSurface();
  ~CRendererMediaCodecSurface() override;

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  bool RenderCapture(int index, CRenderCapture* capture) override;
  void AddVideoPicture(const VideoPicture& picture, int index) override;
  void ReleaseBuffer(int idx) override;
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_bConfigured; }
  bool ConfigChanged(const VideoPicture& picture) override { return false; }
  CRenderInfo GetRenderInfo() override;
  void UnInit() override{};
  void Update() override{};
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool SupportsMultiPassRendering() override { return false; }

  // Player functions
  bool IsGuiLayer() override { return false; }

  // Feature support
  bool Supports(ESCALINGMETHOD method) const override { return false; }
  bool Supports(ERENDERFEATURE feature) const override;

protected:
  void ReorderDrawPoints() override;

private:
  void Reset();
  void ReleaseVideoBuffer(int idx, bool render);

  bool m_bConfigured = false;
  CRect m_surfDestRect;
  int m_lastIndex = -1;

  struct BUFFER
  {
    CVideoBuffer *videoBuffer = nullptr;
  } m_buffers[4];
};
