/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
