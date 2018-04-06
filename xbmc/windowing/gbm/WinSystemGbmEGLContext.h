/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/EGLUtils.h"
#include "WinSystemGbm.h"
#include <memory>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVaapiProxy;

class CWinSystemGbmEGLContext : public CWinSystemGbm
{
public:
  virtual ~CWinSystemGbmEGLContext() = default;

  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;

  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig  GetEGLConfig() const;
protected:
  CWinSystemGbmEGLContext(EGLenum platform, std::string const& platformExtension) :
    m_eglContext(platform, platformExtension)
  {}

  /**
   * Inheriting classes should override InitWindowSystem() without parameters
   * and call this function there with appropriate parameters
   */
  bool InitWindowSystemEGL(EGLint renderableType, EGLint apiType);
  virtual bool CreateContext() = 0;
  virtual void RegisterRenderers() = 0;

  CEGLContextUtils m_eglContext;

  struct delete_CVaapiProxy
  {
    void operator()(CVaapiProxy *p) const;
  };
  std::unique_ptr<CVaapiProxy, delete_CVaapiProxy> m_vaapiProxy;
};

}
}
}
