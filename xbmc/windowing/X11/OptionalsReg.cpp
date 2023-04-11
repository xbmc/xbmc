/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OptionalsReg.h"

#include "cores/VideoPlayer/VideoReferenceClock.h"

//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------
#if defined (HAVE_LIBVA)
#include <va/va_x11.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#if defined(HAS_GL)
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"
#endif
#if defined(HAS_GLES)
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGLES.h"
#endif

using namespace KODI::WINDOWING::X11;

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

class CWinSystemX11GLContext;

class CVaapiProxy : public VAAPI::IVaapiWinSystem
{
public:
  CVaapiProxy() = default;
  virtual ~CVaapiProxy() = default;
  VADisplay GetVADisplay() override { return vaGetDisplay(dpy); };
  void *GetEGLDisplay() override { return eglDisplay; };

  Display *dpy;
  void *eglDisplay;
};

CVaapiProxy* VaapiProxyCreate()
{
  return new CVaapiProxy();
}

void VaapiProxyDelete(CVaapiProxy *proxy)
{
  delete proxy;
}

void VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy)
{
  proxy->dpy = static_cast<Display*>(dpy);
  proxy->eglDisplay = eglDpy;
}

void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{
  VAAPI::CDecoder::Register(winSystem, deepColor);
}

#if defined(HAS_GL)
void VAAPIRegisterRenderGL(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplay(winSystem->dpy);
  CRendererVAAPIGL::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}
#endif

#if defined(HAS_GLES)
void VAAPIRegisterRenderGLES(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
  EGLDisplay eglDpy = winSystem->eglDisplay;
  VADisplay vaDpy = vaGetDisplay(winSystem->dpy);
  CRendererVAAPIGLES::Register(winSystem, vaDpy, eglDpy, general, deepColor);
}
#endif
}
}
}

#else
namespace KODI
{
namespace WINDOWING
{
namespace X11
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

void VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy)
{
}

void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor)
{
}

void VAAPIRegisterRenderGL(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
}

void VAAPIRegisterRenderGLES(CVaapiProxy* winSystem, bool& general, bool& deepColor)
{
}

}
}
}
#endif

//-----------------------------------------------------------------------------
// GLX
//-----------------------------------------------------------------------------

#ifdef HAS_GLX
#include <GL/glx.h>
#include "VideoSyncGLX.h"
#include "GLContextGLX.h"

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

XID GLXGetWindow(void* context)
{
  return static_cast<CGLContextGLX*>(context)->m_glxWindow;
}

void* GLXGetContext(void* context)
{
  return static_cast<CGLContextGLX*>(context)->m_glxContext;
}

CGLContext* GLXContextCreate(Display *dpy)
{
  return new CGLContextGLX(dpy);
}

CVideoSync* GLXVideoSyncCreate(CVideoReferenceClock* clock, CWinSystemX11GLContext& winSystem)
{
  return new  CVideoSyncGLX(clock, winSystem);
}

}
}
}
#else

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

XID GLXGetWindow(void* context)
{
  return 0;
}

void* GLXGetContext(void* context)
{
  return nullptr;
}

CGLContext* GLXContextCreate(Display *dpy)
{
  return nullptr;
}

CVideoSync* GLXVideoSyncCreate(CVideoReferenceClock* clock, CWinSystemX11GLContext& winSystem)
{
  return nullptr;
}

}
}
}
#endif

//-----------------------------------------------------------------------------
// VDPAU
//-----------------------------------------------------------------------------

#if defined (HAVE_LIBVDPAU)
#include "cores/VideoPlayer/DVDCodecs/Video/VDPAU.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVDPAU.h"

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

void VDPAURegisterRender()
{
  CRendererVDPAU::Register();
}

void VDPAURegister()
{
  VDPAU::CDecoder::Register();
}

}
}
}
#else

namespace KODI
{
namespace WINDOWING
{
namespace X11
{

void VDPAURegisterRender()
{

}

void VDPAURegister()
{

}

}
}
}
#endif

