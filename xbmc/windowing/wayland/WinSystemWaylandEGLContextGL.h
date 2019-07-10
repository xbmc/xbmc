/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemWaylandEGLContext.h"
#include "rendering/gl/RenderSystemGL.h"

class CVaapiProxy;

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinSystemWaylandEGLContextGL : public CWinSystemWaylandEGLContext, public CRenderSystemGL
{
public:
  // Implementation of CWinSystemBase via CWinSystemWaylandEGLContext
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;

protected:
  bool CreateContext() override;
  void SetContextSize(CSizeInt size) override;
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;
  struct delete_CVaapiProxy
  {
    void operator()(CVaapiProxy *p) const;
  };
  std::unique_ptr<CVaapiProxy, delete_CVaapiProxy> m_vaapiProxy;
};

}
}
}
