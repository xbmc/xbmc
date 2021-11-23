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
#if defined(HAS_GL)
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"
#endif
#if defined(HAS_GLES)
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGLES.h"
#endif

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

#if defined(HAS_GL)
void VAAPIRegisterRenderGL(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplayWl(winSystem->dpy);
  CRendererVAAPIGL::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}
#endif

#if defined(HAS_GLES)
void VAAPIRegisterRenderGLES(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplayWl(winSystem->dpy);
  CRendererVAAPIGLES::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}
#endif

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

#if defined(HAS_GL)
void VAAPIRegisterRenderGL(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{

}
#endif

#if defined(HAS_GLES)
void VAAPIRegisterRenderGLES(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
}
#endif

} // namespace WAYLAND
} // namespace WINDOWING
} // namespace KODI

#endif

