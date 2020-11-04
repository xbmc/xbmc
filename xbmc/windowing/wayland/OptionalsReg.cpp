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
#if defined(HAVE_LIBVA) && defined(HAS_EGL)
#include <va/va_wayland.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CVaapiProxy : public VAAPI::IVaapiWinSystem
{
public:
  CVaapiProxy() = default;
  virtual ~CVaapiProxy() = default;
  VADisplay GetVADisplay() override { return vaGetDisplayWl(dpy); };
  void *GetEGLDisplay() override { return eglDisplay; };

  wl_display *dpy;
  void *eglDisplay;
};

CVaapiProxy* VaapiProxyCreate()
{
  return new CVaapiProxy();
}

void VaapiProxyDelete(CVaapiProxy* proxy)
{
  delete proxy;
}

void VaapiProxyConfig(CVaapiProxy* proxy, void* dpy, void* eglDpy)
{
  proxy->dpy = static_cast<wl_display*>(dpy);
  proxy->eglDisplay = eglDpy;
}

void VAAPIRegister(CVaapiProxy* winSystem, bool deepColor)
{
  VAAPI::CDecoder::Register(winSystem, deepColor);
}

void VAAPIRegisterRender(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplayWl(winSystem->dpy);
  CRendererVAAPI::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}

} // namespace WAYLAND
} // namespace WINDOWING
} // namespace KODI

#else

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CVaapiProxy
{
};

CVaapiProxy* VaapiProxyCreate()
{
  return nullptr;
}

void VaapiProxyDelete(CVaapiProxy* proxy)
{
}

void VaapiProxyConfig(CVaapiProxy* proxy, void* dpy, void* eglDpy)
{

}

void VAAPIRegister(CVaapiProxy* winSystem, bool deepColor)
{

}

void VAAPIRegisterRender(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{

}


} // namespace WAYLAND
} // namespace WINDOWING
} // namespace KODI

#endif

