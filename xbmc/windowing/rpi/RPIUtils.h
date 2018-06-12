/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "platform/linux/RBP.h"
#include "EGL/egl.h"
#include <bcm_host.h>
#include "windowing/Resolution.h"

class DllBcmHost;
class CRPIUtils
{
public:
  CRPIUtils();
  virtual ~CRPIUtils();
  virtual void DestroyDispmanxWindow();
  virtual void SetVisible(bool enable);
  virtual bool GetNativeResolution(RESOLUTION_INFO *res) const;
  virtual bool SetNativeResolution(const RESOLUTION_INFO res, EGLSurface m_nativeWindow);
  virtual bool ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);
private:
  DllBcmHost *m_DllBcmHost;
  DISPMANX_ELEMENT_HANDLE_T m_dispman_display;
  DISPMANX_ELEMENT_HANDLE_T m_dispman_element;
  TV_GET_STATE_RESP_T m_tv_state;
  sem_t m_tv_synced;
  RESOLUTION_INFO m_desktopRes;
  int m_width;
  int m_height;
  int m_screen_width;
  int m_screen_height;
  bool m_shown;

  int m_initDesktopRes;

  void GetSupportedModes(HDMI_RES_GROUP_T group, std::vector<RESOLUTION_INFO> &resolutions);
  void TvServiceCallback(uint32_t reason, uint32_t param1, uint32_t param2);
  static void CallbackTvServiceCallback(void *userdata, uint32_t reason, uint32_t param1, uint32_t param2);

  int FindMatchingResolution(const RESOLUTION_INFO &res, const std::vector<RESOLUTION_INFO> &resolutions, bool desktop);
  int AddUniqueResolution(RESOLUTION_INFO &res, std::vector<RESOLUTION_INFO> &resolutions, bool desktop = false);
};
