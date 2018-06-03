/*
 *      Copyright (C) 2007-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"

class CRendererAML : public CBaseRenderer
{
public:
  CRendererAML();
  virtual ~CRendererAML();

  // Registration
  static CBaseRenderer* Create(CVideoBuffer *buffer);
  static bool Register();

  virtual bool RenderCapture(CRenderCapture* capture) override;
  virtual void AddVideoPicture(const VideoPicture &picture, int index, double currentClock) override;
  virtual void ReleaseBuffer(int idx) override;
  virtual bool Configure(const VideoPicture &picture, float fps, unsigned int orientation) override;
  virtual bool IsConfigured() override { return m_bConfigured; };
  virtual bool ConfigChanged(const VideoPicture &picture) { return false; };
  virtual CRenderInfo GetRenderInfo() override;
  virtual void UnInit() override {};
  virtual void Update() override {};
  virtual void RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha) override;
  virtual bool SupportsMultiPassRendering()override { return false; };

  // Player functions
  virtual bool IsGuiLayer() override { return false; };

  // Feature support
  virtual bool Supports(ESCALINGMETHOD method) override { return false; };
  virtual bool Supports(ERENDERFEATURE feature) override;

private:
  void Reset();

  static const int m_numRenderBuffers = 4;

  struct BUFFER
  {
    BUFFER() : videoBuffer(nullptr) {};
    CVideoBuffer *videoBuffer;
    int duration;
  } m_buffers[m_numRenderBuffers];

  int m_prevVPts;
  bool m_bConfigured;
};
