/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "system.h"
#include "system_gl.h"
#include "rendering/RenderSystem.h"

class CRenderSystemGL : public CRenderSystemBase
{
public:
  CRenderSystemGL();
  virtual ~CRenderSystemGL();
  void CheckOpenGLQuirks();
  bool InitRenderSystem() override;
  bool DestroyRenderSystem() override;
  bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate) override;

  bool BeginRender() override;
  bool EndRender() override;
  void PresentRender(bool rendered, bool videoLayer) override;
  bool ClearBuffers(color_t color) override;
  bool IsExtSupported(const char* extension) override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }
  void FinishPipeline() override;

  void SetViewPort(CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  void SetScissors(const CRect &rect) override;
  void ResetScissors() override;

  void CaptureStateBlock() override;
  void ApplyStateBlock() override;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;

  void ApplyHardwareTransform(const TransformMatrix &matrix) override;
  void RestoreHardwareTransform() override;
  void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view) override;
  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;

  bool TestRender() override;

  void Project(float &x, float &y, float &z) override;

  void GetGLSLVersion(int& major, int& minor);

  void ResetGLErrors();

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual void PresentRenderImpl(bool rendered) = 0;
  void CalculateMaxTexturesize();

  bool m_bVsyncInit;
  int m_width;
  int m_height;

  std::string m_RenderExtensions;

  int m_glslMajor = 0;
  int m_glslMinor = 0;
  
  GLint m_viewPort[4];

  uint8_t m_latencyCounter = 0;
};
