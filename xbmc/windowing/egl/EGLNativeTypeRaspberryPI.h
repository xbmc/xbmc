#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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

#include "EGLNativeType.h"
#if defined(TARGET_RASPBERRY_PI)
#include <semaphore.h>
#include <bcm_host.h>
#endif

class DllBcmHost;
class CEGLNativeTypeRaspberryPI : public CEGLNativeType
{
public:
  CEGLNativeTypeRaspberryPI();
  virtual ~CEGLNativeTypeRaspberryPI();
  virtual std::string GetNativeName() const { return "raspberrypi"; };
  virtual bool  CheckCompatibility();
  virtual void  Initialize();
  virtual void  Destroy();
  virtual int   GetQuirks() { return EGL_QUIRK_NONE; };

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
#if defined(TARGET_RASPBERRY_PI)
private:
  DllBcmHost                    *m_DllBcmHost;
  DISPMANX_ELEMENT_HANDLE_T     m_dispman_display;
  DISPMANX_ELEMENT_HANDLE_T     m_dispman_element;
  TV_GET_STATE_RESP_T           m_tv_state;
  sem_t                         m_tv_synced;
  bool                          m_fixedMode;
  RESOLUTION_INFO               m_desktopRes;
  int                           m_width;
  int                           m_height;
  int                           m_initDesktopRes;

  void GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions);
  void TvServiceCallback(uint32_t reason, uint32_t param1, uint32_t param2);
  static void CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2);

  void DestroyDispmaxWindow();
  int FindMatchingResolution(const RESOLUTION_INFO &res, const std::vector<RESOLUTION_INFO> &resolutions);
  int AddUniqueResolution(RESOLUTION_INFO &res, std::vector<RESOLUTION_INFO> &resolutions);
#endif
};
