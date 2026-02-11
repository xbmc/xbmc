/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderSystemTypes.h"
#include "threads/CriticalSection.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <memory>
#include <optional>
#include <string>

/*
 *   CRenderSystemBase interface allows us to create the rendering engine we use.
 *   We currently have two engines: OpenGL and DirectX
 *   This interface is very basic since a lot of the actual details will go in to the derived classes
 */

enum class DepthCulling
{
  OFF,
  BACK_TO_FRONT,
  FRONT_TO_BACK,
};

enum class ClearFunction
{
  FIXED_FUNCTION,
  GEOMETRY,
};

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
  virtual void InvalidateColorBuffer() {}
  virtual bool ClearBuffers(KODI::UTILS::COLOR::Color color) = 0;
  virtual bool IsExtSupported(const char* extension) const = 0;

  virtual void SetViewPort(const CRect& viewPort) = 0;
  virtual void GetViewPort(CRect& viewPort) = 0;
  virtual void RestoreViewPort() {}

  virtual bool ScissorsCanEffectClipping() { return false; }
  virtual CRect ClipRectToScissorRect(const CRect& rect) { return CRect(); }
  virtual void SetScissors(const CRect& rect) = 0;
  virtual void ResetScissors() = 0;

  virtual void SetDepthCulling(DepthCulling culling) {}

  virtual void CaptureStateBlock() = 0;
  virtual void ApplyStateBlock() = 0;

  virtual void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.f) = 0;
  virtual void SetStereoMode(RenderStereoMode mode, RenderStereoView view)
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
  virtual bool SupportsStereo(RenderStereoMode mode) const;
  unsigned int GetMaxTextureSize() const { return m_maxTextureSize; }
  unsigned int GetMinDXTPitch() const { return m_minDXTPitch; }

  virtual void ShowSplash(const std::string& message);

  /*!
   * \brief Call when the cached advanced settings values need to be refreshed.
   *        note: may execute on a different thread.
   */
  virtual void OnAdvancedSettingsLoaded();
  virtual bool GetEnabledFrontToBackRendering();
  virtual ClearFunction GetClearFunction();
  virtual bool GetShowSplashImage();

protected:
  bool m_bRenderCreated{false};
  bool m_bVSync{true};
  unsigned int m_maxTextureSize{2048};
  unsigned int m_minDXTPitch{0};

  std::string m_RenderRenderer;
  std::string m_RenderVendor;
  std::string m_RenderVersion;
  int m_RenderVersionMinor{0};
  int m_RenderVersionMajor{0};
  RenderStereoView m_stereoView{RenderStereoView::OFF};
  RenderStereoMode m_stereoMode{RenderStereoMode::OFF};
  bool m_limitedColorRange{false};
  bool m_transferPQ{false};

  std::unique_ptr<CGUIImage> m_splashImage;
  std::unique_ptr<CGUITextLayout> m_splashMessageLayout;

  // Advanced settings handling
  CCriticalSection m_settingsSection;
  std::optional<int> m_settingsCallbackHandle;
  bool m_guiFrontToBackRendering{false};
  ClearFunction m_guiGeometryClear{ClearFunction::FIXED_FUNCTION};
  bool m_showSplashImage{true};
};
