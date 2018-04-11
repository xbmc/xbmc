/*
 *      Copyright (C) 2005-2017 Team XBMC
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

#include "OptionalsReg.h"

//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------
#if defined (HAVE_LIBVA)
#include <va/va_wayland.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"

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

CVaapiProxy* WAYLAND::VaapiProxyCreate()
{
  return new CVaapiProxy();
}

void WAYLAND::VaapiProxyDelete(CVaapiProxy *proxy)
{
  delete proxy;
}

void WAYLAND::VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy)
{
  proxy->dpy = static_cast<wl_display*>(dpy);
  proxy->eglDisplay = eglDpy;
}

void WAYLAND::VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{
  VAAPI::CDecoder::Register(winSystem, deepColor);
}

void WAYLAND::VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplayWl(winSystem->dpy);
  CRendererVAAPI::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}

#else

class CVaapiProxy
{
};

CVaapiProxy* WAYLAND::VaapiProxyCreate()
{
  return nullptr;
}

void WAYLAND::VaapiProxyDelete(CVaapiProxy *proxy)
{
}

void WAYLAND::VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy)
{

}

void WAYLAND::VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{

}

void WAYLAND::VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{

}
#endif

