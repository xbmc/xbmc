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

#pragma once

#include "X11/Xlib.h"

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

//-----------------------------------------------------------------------------
// ALSA
//-----------------------------------------------------------------------------

namespace X11
{
bool ALSARegister();
}

//-----------------------------------------------------------------------------
// PulseAudio
//-----------------------------------------------------------------------------

namespace X11
{
bool PulseAudioRegister();
}

//-----------------------------------------------------------------------------
// sndio
//-----------------------------------------------------------------------------

namespace X11
{
bool SndioRegister();
}
