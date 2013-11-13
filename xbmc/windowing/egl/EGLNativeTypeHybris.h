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

#if defined(TARGET_HYBRIS)
#include <hwcomposerwindow/hwcomposer_window.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#endif

#include "EGLNativeType.h"
#include "threads/Thread.h"

class CEGLNativeTypeHybris;

class CHybrisVideoRenderer : public CThread
{
public:
  CHybrisVideoRenderer(hwc_display_contents_1_t **bufferList,
    hwc_composer_device_1_t *hwcDevicePtr,
    HWComposerNativeWindow *nativeWindow);
  virtual ~CHybrisVideoRenderer();
private:
  hwc_display_contents_1_t   **m_bufferList;
  hwc_composer_device_1_t    *m_hwcDevicePtr;
  HWComposerNativeWindow     *m_hwNativeWindow;
protected:
  void Process();
};

class CEGLNativeTypeHybris : public CEGLNativeType
{
public:
  CEGLNativeTypeHybris();
  virtual ~CEGLNativeTypeHybris();
  virtual std::string GetNativeName() const { return "hybris"; };
  virtual bool  CheckCompatibility();
  virtual void  Initialize();
  virtual void  Destroy();
  virtual int   GetQuirks() { return 0; };

  virtual bool  CreateNativeDisplay();
  virtual bool  CreateNativeWindow();
  virtual bool  GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const;
  virtual bool  GetNativeWindow(XBNativeWindowType **nativeWindow) const;

  virtual bool  DestroyNativeWindow();
  virtual bool  DestroyNativeDisplay();

  virtual bool  GetNativeResolution(RESOLUTION_INFO *res) const;
  virtual bool  SetNativeResolution(const RESOLUTION_INFO &res);
  virtual bool  ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);
  virtual bool  GetPreferredResolution(RESOLUTION_INFO *res) const;

  virtual bool  ShowWindow(bool show);
  void SwapSurface(EGLDisplay display, EGLSurface surface);
#if defined(TARGET_HYBRIS)
private:
  hw_module_t                *m_hwcModule;
  hwc_display_contents_1_t   **m_bufferList;
  hwc_composer_device_1_t    *m_hwcDevicePtr;
  HWComposerNativeWindow     *m_hwNativeWindow;
  ANativeWindow              *m_swNativeWindow;
#endif
  CHybrisVideoRenderer       *m_videoRenderThread;
};
