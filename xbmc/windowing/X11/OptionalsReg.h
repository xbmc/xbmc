/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <X11/Xlib.h>

//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------

class CVaapiProxy;

namespace X11
{
CVaapiProxy* VaapiProxyCreate();
void VaapiProxyDelete(CVaapiProxy *proxy);
void VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy);
void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor);
void VAAPIRegisterRender(CVaapiProxy *winSystem, bool &general, bool &deepColor);
}

//-----------------------------------------------------------------------------
// GLX
//-----------------------------------------------------------------------------

class CVideoSync;
class CGLContext;
class CWinSystemX11GLContext;

namespace X11
{
XID GLXGetWindow(void* context);
void* GLXGetContext(void* context);
CGLContext* GLXContextCreate(Display *dpy);
CVideoSync* GLXVideoSyncCreate(void *clock, CWinSystemX11GLContext& winSystem);
}

//-----------------------------------------------------------------------------
// VDPAU
//-----------------------------------------------------------------------------

namespace X11
{
void VDPAURegisterRender();
void VDPAURegister();
}
