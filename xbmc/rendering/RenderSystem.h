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

#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#pragma once

#include "guilib/Geometry.h"
#include "guilib/TransformMatrix.h"
#include "guilib/DirtyRegion.h"
#include <stdint.h>
#include <string>

typedef enum _RenderingSystemType
{
  RENDERING_SYSTEM_OPENGL,
  RENDERING_SYSTEM_DIRECTX,
  RENDERING_SYSTEM_OPENGLES
} RenderingSystemType;

/*
*   CRenderSystemBase interface allows us to create the rendering engine we use.
*   We currently have two engines: OpenGL and DirectX
*   This interface is very basic since a lot of the actual details will go in to the derived classes
*/

typedef uint32_t color_t;

enum
{
  RENDER_CAPS_DXT      = (1 << 0),
  RENDER_CAPS_NPOT     = (1 << 1),
  RENDER_CAPS_DXT_NPOT = (1 << 2),
  RENDER_CAPS_BGRA     = (1 << 3),
  RENDER_CAPS_BGRA_APPLE = (1 << 4)
};

enum
{
  RENDER_QUIRKS_MAJORMEMLEAK_OVERLAYRENDERER = 1 << 0,
  RENDER_QUIRKS_YV12_PREFERED                = 1 << 1,
  RENDER_QUIRKS_BROKEN_OCCLUSION_QUERY       = 1 << 2,
};

enum RENDER_STEREO_VIEW
{
  RENDER_STEREO_VIEW_OFF,
  RENDER_STEREO_VIEW_LEFT,
  RENDER_STEREO_VIEW_RIGHT,
};

enum RENDER_STEREO_MODE
{
  RENDER_STEREO_MODE_OFF,
  RENDER_STEREO_MODE_SPLIT_HORIZONTAL,
  RENDER_STEREO_MODE_SPLIT_VERTICAL,
  RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN,
  RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA,
  RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE,
  RENDER_STEREO_MODE_INTERLACED,
  RENDER_STEREO_MODE_HARDWAREBASED,
  RENDER_STEREO_MODE_MONO,
  RENDER_STEREO_MODE_COUNT,

  // psuevdo modes
  RENDER_STEREO_MODE_AUTO = 100,
  RENDER_STEREO_MODE_UNDEFINED = 999,
};


class CRenderSystemBase
{
public:
  CRenderSystemBase();
  virtual ~CRenderSystemBase();

  // Retrieve
  RenderingSystemType GetRenderingSystemType() { return m_enumRenderingSystem; }

  virtual bool InitRenderSystem() = 0;
  virtual bool DestroyRenderSystem() = 0;
  virtual bool ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate) = 0;

  virtual bool BeginRender() = 0;
  virtual bool EndRender() = 0;
  virtual bool PresentRender(const CDirtyRegionList& dirty) = 0;
  virtual bool ClearBuffers(color_t color) = 0;
  virtual bool IsExtSupported(const char* extension) = 0;

  virtual void SetVSync(bool vsync) = 0;
  bool GetVSync() { return m_bVSync; }

  virtual void SetViewPort(CRect& viewPort) = 0;
  virtual void GetViewPort(CRect& viewPort) = 0;
  virtual void RestoreViewPort() {};

  virtual bool ScissorsCanEffectClipping() { return false; }
  virtual CRect ClipRectToScissorRect(const CRect &rect) { return CRect(); }
  virtual void SetScissors(const CRect &rect) = 0;
  virtual void ResetScissors() = 0;

  virtual void CaptureStateBlock() = 0;
  virtual void ApplyStateBlock() = 0;

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight) = 0;
  virtual void ApplyHardwareTransform(const TransformMatrix &matrix) = 0;
  virtual void RestoreHardwareTransform() = 0;
  virtual void SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
  {
    m_stereoMode = mode;
    m_stereoView = view;
  }

  virtual bool TestRender() = 0;

  /**
   * Project (x,y,z) 3d scene coordinates to (x,y) 2d screen coordinates
   */
  virtual void Project(float &x, float &y, float &z) { }

  void GetRenderVersion(unsigned int& major, unsigned int& minor) const;
  const std::string& GetRenderVendor() const { return m_RenderVendor; }
  const std::string& GetRenderRenderer() const { return m_RenderRenderer; }
  const std::string& GetRenderVersionString() const { return m_RenderVersion; }
  bool SupportsDXT() const;
  bool SupportsBGRA() const;
  bool SupportsBGRAApple() const;
  bool SupportsNPOT(bool dxt) const;
  bool SupportsStereo(RENDER_STEREO_MODE mode) const;
  unsigned int GetMaxTextureSize() const { return m_maxTextureSize; }
  unsigned int GetMinDXTPitch() const { return m_minDXTPitch; }
  unsigned int GetRenderQuirks() const { return m_renderQuirks; }

protected:
  bool                m_bRenderCreated;
  RenderingSystemType m_enumRenderingSystem;
  bool                m_bVSync;
  unsigned int        m_maxTextureSize;
  unsigned int        m_minDXTPitch;

  std::string   m_RenderRenderer;
  std::string   m_RenderVendor;
  std::string   m_RenderVersion;
  int          m_RenderVersionMinor;
  int          m_RenderVersionMajor;
  unsigned int m_renderCaps;
  unsigned int m_renderQuirks;
  RENDER_STEREO_VIEW m_stereoView;
  RENDER_STEREO_MODE m_stereoMode;
};

#endif // RENDER_SYSTEM_H
