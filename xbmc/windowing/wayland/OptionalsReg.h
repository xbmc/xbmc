/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once


//-----------------------------------------------------------------------------
// VAAPI
//-----------------------------------------------------------------------------

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{
class CVaapiProxy;

CVaapiProxy* VaapiProxyCreate();
void VaapiProxyDelete(CVaapiProxy *proxy);
void VaapiProxyConfig(CVaapiProxy *proxy, void *dpy, void *eglDpy);
void VAAPIRegister(CVaapiProxy *winSystem, bool deepColor);
#if defined(HAS_GL)
void VAAPIRegisterRenderGL(CVaapiProxy* winSystem, bool& general, bool& deepColor);
#endif
#if defined(HAS_GLES)
void VAAPIRegisterRenderGLES(CVaapiProxy* winSystem, bool& general, bool& deepColor);
#endif

} // namespace WAYLAND
} // namespace WINDOWING
} // namespace KODI
