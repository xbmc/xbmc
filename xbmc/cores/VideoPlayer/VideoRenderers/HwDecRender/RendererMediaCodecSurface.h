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
  virtual ~CRendererMediaCodecSurface();

  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  virtual bool RenderCapture(CRenderCapture* capture) override;
  virtual void AddVideoPicture(const VideoPicture &picture, int index) override;
  virtual void ReleaseBuffer(int idx) override;
  virtual bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;
  virtual bool IsConfigured() override { return m_bConfigured; };
  virtual bool ConfigChanged(const VideoPicture &picture) override { return false; };
  virtual CRenderInfo GetRenderInfo() override;
  virtual void UnInit() override {};
  virtual void Update() override {};
  virtual void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  virtual bool SupportsMultiPassRendering() override { return false; };

  // Player functions
  virtual bool IsGuiLayer() override { return false; };

  // Feature support
  virtual bool Supports(ESCALINGMETHOD method) override { return false; };
  virtual bool Supports(ERENDERFEATURE feature) override;

protected:
  virtual void ReorderDrawPoints() override;

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
