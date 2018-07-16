/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
