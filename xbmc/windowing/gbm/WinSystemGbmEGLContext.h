/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemGbm.h"
#include "utils/EGLFence.h"
#include "utils/EGLUtils.h"
#include "windowing/linux/WinSystemEGL.h"

#include <memory>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVaapiProxy;

class CWinSystemGbmEGLContext : public KODI::WINDOWING::LINUX::CWinSystemEGL, public CWinSystemGbm
{
public:
  ~CWinSystemGbmEGLContext() override = default;

  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;

protected:
  CWinSystemGbmEGLContext(EGLenum platform, std::string const& platformExtension)
    : CWinSystemEGL{platform, platformExtension}
  {}

  /**
   * Inheriting classes should override InitWindowSystem() without parameters
   * and call this function there with appropriate parameters
   */
  bool InitWindowSystemEGL(EGLint renderableType, EGLint apiType);
  virtual bool CreateContext() = 0;

  std::unique_ptr<KODI::UTILS::EGL::CEGLFence> m_eglFence;

  struct delete_CVaapiProxy
  {
    void operator()(CVaapiProxy *p) const;
  };
  std::unique_ptr<CVaapiProxy, delete_CVaapiProxy> m_vaapiProxy;
};

}
}
}
