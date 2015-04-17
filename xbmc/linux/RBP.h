#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef USE_VCHIQ_ARM
#define USE_VCHIQ_ARM
#endif
#ifndef __VIDEOCORE4__
#define __VIDEOCORE4__
#endif
#ifndef HAVE_VMCS_CONFIG
#define HAVE_VMCS_CONFIG
#endif

#if defined(HAVE_CONFIG_H) && !defined(TARGET_WINDOWS)
#include "config.h"
#define DECLARE_UNUSED(a,b) a __attribute__((unused)) b;
#endif

#if defined(TARGET_RASPBERRY_PI)
#include "DllBCM.h"
#include "OMXCore.h"
#include "xbmc/utils/CPUInfo.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

class CRBP
{
public:
  CRBP();
  ~CRBP();

  bool Initialize();
  void LogFirmwareVerison();
  void Deinitialize();
  int GetArmMem() { return m_arm_mem; }
  int GetGpuMem() { return m_gpu_mem; }
  bool GetCodecMpg2() { return m_codec_mpg2_enabled; }
  int RasberryPiVersion() { return g_cpuInfo.getCPUCount() == 1 ? 1 : 2; };
  bool GetCodecWvc1() { return m_codec_wvc1_enabled; }
  void GetDisplaySize(int &width, int &height);
  DISPMANX_DISPLAY_HANDLE_T OpenDisplay(uint32_t device);
  void CloseDisplay(DISPMANX_DISPLAY_HANDLE_T display);
  int GetGUIResolutionLimit() { return m_gui_resolution_limit; }
  // stride can be null for packed output
  unsigned char *CaptureDisplay(int width, int height, int *stride, bool swap_red_blue, bool video_only = true);
  DllOMX *GetDllOMX() { return m_OMX ? m_OMX->GetDll() : NULL; }
  void WaitVsync();

private:
  DllBcmHost *m_DllBcmHost;
  bool       m_initialized;
  bool       m_omx_initialized;
  bool       m_omx_image_init;
  int        m_arm_mem;
  int        m_gpu_mem;
  int        m_gui_resolution_limit;
  bool       m_codec_mpg2_enabled;
  bool       m_codec_wvc1_enabled;
  COMXCore   *m_OMX;
  DISPMANX_DISPLAY_HANDLE_T m_display;
  CEvent     m_vsync;
  class DllLibOMXCore;
  CCriticalSection m_critSection;
};

extern CRBP g_RBP;
#endif
