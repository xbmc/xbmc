/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OptionalsReg.h"

//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------
#if defined (HAVE_LIBVA)
#include <va/va_drm.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGLES.h"

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVaapiProxy : public VAAPI::IVaapiWinSystem
{
public:
  CVaapiProxy(int fd) : m_fd(fd) {};
  virtual ~CVaapiProxy() = default;
  VADisplay GetVADisplay() override;
  void *GetEGLDisplay() override { return eglDisplay; };

  VADisplay vaDpy;
  void *eglDisplay;

private:
  int m_fd{-1};
};

VADisplay CVaapiProxy::GetVADisplay()
{
  return vaGetDisplayDRM(m_fd);
}

CVaapiProxy* VaapiProxyCreate(int fd)
{
  return new CVaapiProxy(fd);
}

void VaapiProxyDelete(CVaapiProxy *proxy)
{
  delete proxy;
}

void VaapiProxyConfig(CVaapiProxy *proxy, void *eglDpy)
{
  proxy->vaDpy = proxy->GetVADisplay();
  proxy->eglDisplay = eglDpy;
}

void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{
  VAAPI::CDecoder::Register(winSystem, deepColor);
}

void VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{
  CRendererVAAPI::Register(winSystem, winSystem->vaDpy, winSystem->eglDisplay, general, deepColor);
}

}
}
}

#else

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVaapiProxy
{
};

CVaapiProxy* VaapiProxyCreate(int fd)
{
  return nullptr;
}

void VaapiProxyDelete(CVaapiProxy *proxy)
{
}

void VaapiProxyConfig(CVaapiProxy *proxy, void *eglDpy)
{

}

void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{

}

void VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{

}

}
}
}

#endif
