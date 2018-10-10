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

CVaapiProxy* VaapiProxyCreate()
{
  return new CVaapiProxy();
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

CVaapiProxy* VaapiProxyCreate()
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
