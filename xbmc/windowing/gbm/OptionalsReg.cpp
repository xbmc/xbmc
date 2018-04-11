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
#include <va/va_drm.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGLES.h"

class CVaapiProxy : public VAAPI::IVaapiWinSystem
{
public:
  CVaapiProxy() = default;
  virtual ~CVaapiProxy() = default;
  VADisplay GetVADisplay() override;
  void *GetEGLDisplay() override { return eglDisplay; };

  VADisplay vaDpy;
  void *eglDisplay;
};

VADisplay CVaapiProxy::GetVADisplay()
{
  int const buf_size{128};
  char name[buf_size];
  int fd{-1};

  // 128 is the start of the NUM in renderD<NUM>
  for (int i = 128; i < (128 + 16); i++)
  {
    snprintf(name, buf_size, "/dev/dri/renderD%u", i);

    fd = open(name, O_RDWR);

    if (fd < 0)
    {
      continue;
    }

    auto display = vaGetDisplayDRM(fd);

    if (display != nullptr)
    {
      return display;
    }
  }

  return nullptr;
}

CVaapiProxy* GBM::VaapiProxyCreate()
{
  return new CVaapiProxy();
}

void GBM::VaapiProxyDelete(CVaapiProxy *proxy)
{
  delete proxy;
}

void GBM::VaapiProxyConfig(CVaapiProxy *proxy, void *eglDpy)
{
  proxy->vaDpy = proxy->GetVADisplay();
  proxy->eglDisplay = eglDpy;
}

void GBM::VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{
  VAAPI::CDecoder::Register(winSystem, deepColor);
}

void GBM::VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{
  CRendererVAAPI::Register(winSystem, winSystem->vaDpy, winSystem->eglDisplay, general, deepColor);
}

#else

class CVaapiProxy
{
};

CVaapiProxy* GBM::VaapiProxyCreate()
{
  return nullptr;
}

void GBM::VaapiProxyDelete(CVaapiProxy *proxy)
{
}

void GBM::VaapiProxyConfig(CVaapiProxy *proxy, void *eglDpy)
{

}

void GBM::VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{

}

void GBM::VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor)
{

}
#endif
