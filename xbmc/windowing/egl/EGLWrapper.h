#pragma once

/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "guilib/Resolution.h"
#include <EGL/egl.h>
class CEGLNativeType;
class CEGLWrapper
{
public:
  CEGLWrapper();
  ~CEGLWrapper();

  bool Initialize(const std::string &implementation = "auto");
  bool Destroy();
  std::string GetNativeName();

  bool CreateNativeDisplay();
  bool CreateNativeWindow();
  void DestroyNativeDisplay();
  void DestroyNativeWindow();

  bool SetNativeResolution(RESOLUTION_INFO& res);
  bool ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);
  bool ShowWindow(bool show);
  bool GetQuirks(int *quirks);
  bool GetPreferredResolution(RESOLUTION_INFO *res);
  bool GetNativeResolution(RESOLUTION_INFO *res);

  bool InitDisplay(EGLDisplay *display);
  bool ChooseConfig(EGLDisplay display, EGLint *configAttrs, EGLConfig *config);
  bool CreateContext(EGLDisplay display, EGLConfig config, EGLint *contextAttrs, EGLContext *context);
  bool CreateSurface(EGLDisplay display, EGLConfig config, EGLSurface *surface);
  bool GetSurfaceSize(EGLDisplay display, EGLSurface surface, EGLint *width, EGLint *height);
  bool BindContext(EGLDisplay display, EGLSurface surface, EGLContext context);
  bool BindAPI(EGLint type);
  bool ReleaseContext(EGLDisplay display);
  bool DestroyContext(EGLDisplay display, EGLContext context);
  bool DestroySurface(EGLSurface surface, EGLDisplay display);
  bool DestroyDisplay(EGLDisplay display);

  std::string GetExtensions(EGLDisplay display);
  void SwapBuffers(EGLDisplay display, EGLSurface surface);
  bool SetVSync(EGLDisplay display, bool enable);
  bool IsExtSupported(const char* extension);
  bool GetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value);

private:
    CEGLNativeType          *m_nativeTypes;
    EGLint                  m_result;
};

