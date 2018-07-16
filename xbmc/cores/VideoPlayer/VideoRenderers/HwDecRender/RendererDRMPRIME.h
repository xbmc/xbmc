/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "windowing/gbm/WinSystemGbmGLESContext.h"

class CRendererDRMPRIME
  : public CBaseRenderer
{
public:
  CRendererDRMPRIME(std::shared_ptr<CDRMUtils> drm);
  ~CRendererDRMPRIME();

  // Registration
  static CBaseRenderer* Create(CVideoBuffer* buffer);
  static void Register();

  // Player functions
  bool Configure(const VideoPicture& picture, float fps, unsigned int orientation) override;
  bool IsConfigured() override { return m_bConfigured; };
  void AddVideoPicture(const VideoPicture& picture, int index, double currentClock) override;
  void UnInit() override {};
  void Flush() override;
  void ReleaseBuffer(int idx) override;
  bool NeedBuffer(int idx) override;
  bool IsGuiLayer() override { return false; };
  CRenderInfo GetRenderInfo() override;
  void Update() override;
  void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  bool RenderCapture(CRenderCapture* capture) override;
  bool ConfigChanged(const VideoPicture& picture) override;

  // Feature support
  bool SupportsMultiPassRendering() override { return false; };
  bool Supports(ERENDERFEATURE feature) override;
  bool Supports(ESCALINGMETHOD method) override;

protected:
  void ManageRenderArea() override;

private:
  void Reset();
  void SetVideoPlane(CVideoBufferDRMPRIME* buffer);

  bool m_bConfigured = false;
  int m_iLastRenderBuffer = -1;

  std::shared_ptr<CDRMUtils> m_DRM;

  struct BUFFER
  {
    CVideoBuffer* videoBuffer = nullptr;
  } m_buffers[NUM_BUFFERS];
};
