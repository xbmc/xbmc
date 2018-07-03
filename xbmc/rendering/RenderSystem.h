/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "RenderSystemTypes.h"
#include "utils/Color.h"
#include "utils/Geometry.h"
#include "utils/TransformMatrix.h"
#include "guilib/DirtyRegion.h"
#include <memory>
#include <string>

/*
 *   CRenderSystemBase interface allows us to create the rendering engine we use.
 *   We currently have two engines: OpenGL and DirectX
 *   This interface is very basic since a lot of the actual details will go in to the derived classes
 */

class CGUIImage;
class CGUITextLayout;

class CRenderSystemBase
{
public:
  CRenderSystemBase();
  virtual ~CRenderSystemBase();

  virtual bool InitRenderSystem() = 0;
  virtual bool DestroyRenderSystem() = 0;
  virtual bool ResetRenderSystem(int width, int height) = 0;

  virtual bool BeginRender() = 0;
  virtual bool EndRender() = 0;
  virtual void PresentRender(bool rendered, bool videoLayer) = 0;
  virtual bool ClearBuffers(UTILS::Color color) = 0;
  virtual bool IsExtSupported(const char* extension) const = 0;

  virtual void SetViewPort(const CRect& viewPort) = 0;
  virtual void GetViewPort(CRect& viewPort) = 0;
  virtual void RestoreViewPort() {};

  virtual bool ScissorsCanEffectClipping() { return false; }
  virtual CRect ClipRectToScissorRect(const CRect &rect) { return CRect(); }
  virtual void SetScissors(const CRect &rect) = 0;
  virtual void ResetScissors() = 0;

  virtual void CaptureStateBlock() = 0;
  virtual void ApplyStateBlock() = 0;

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.f) = 0;
  virtual void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
  {
    m_stereoMode = mode;
    m_stereoView = view;
  }

  /**
   * Project (x,y,z) 3d scene coordinates to (x,y) 2d screen coordinates
   */
  virtual void Project(float &x, float &y, float &z) { }

  virtual std::string GetShaderPath(const std::string &filename) { return ""; }

  void GetRenderVersion(unsigned int& major, unsigned int& minor) const;
  const std::string& GetRenderVendor() const { return m_RenderVendor; }
  const std::string& GetRenderRenderer() const { return m_RenderRenderer; }
  const std::string& GetRenderVersionString() const { return m_RenderVersion; }
  virtual bool SupportsNPOT(bool dxt) const;
  virtual bool SupportsStereo(RENDER_STEREO_MODE mode) const;
  unsigned int GetMaxTextureSize() const { return m_maxTextureSize; }
  unsigned int GetMinDXTPitch() const { return m_minDXTPitch; }

  virtual void ShowSplash(const std::string& message);

protected:
  bool                m_bRenderCreated;
  bool                m_bVSync;
  unsigned int        m_maxTextureSize;
  unsigned int        m_minDXTPitch;

  std::string   m_RenderRenderer;
  std::string   m_RenderVendor;
  std::string   m_RenderVersion;
  int          m_RenderVersionMinor;
  int          m_RenderVersionMajor;
  RENDER_STEREO_VIEW m_stereoView = RENDER_STEREO_VIEW_OFF;
  RENDER_STEREO_MODE m_stereoMode = RENDER_STEREO_MODE_OFF;
  bool m_limitedColorRange = false;

  std::unique_ptr<CGUIImage> m_splashImage;
  std::unique_ptr<CGUITextLayout> m_splashMessageLayout;
};

