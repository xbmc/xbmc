/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef USE_VCHIQ_ARM
#define USE_VCHIQ_ARM
#endif
#ifndef __VIDEOCORE4__
#define __VIDEOCORE4__
#endif
#ifndef HAVE_VMCS_CONFIG
#define HAVE_VMCS_CONFIG
#endif

#include "DllBCM.h"
#include "OMXCore.h"
#include "xbmc/utils/CPUInfo.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"


class AVRpiZcFrameGeometry
{
public:
  unsigned int getStrideY() { return stride_y; }
  unsigned int getHeightY() { return height_y; }
  unsigned int getStrideC() { return stride_c; }
  unsigned int getHeightC() { return height_c; }
  unsigned int getPlanesC() { return planes_c; }
  unsigned int getStripes() { return stripes; }
  unsigned int getBitsPerPixel() { return bits_per_pixel; }
  unsigned int getBytesPerPixel() { return (bits_per_pixel + 7) >> 3; }
  unsigned int getSizeY() { return stride_y * height_y; }
  unsigned int getSizeC() { return stride_c * height_c; }
  unsigned int getSize() { return (getSizeY() + getSizeC() * getPlanesC()) * getStripes(); }
  void setStrideY(unsigned int v) { stride_y = v; }
  void setHeightY(unsigned int v) { height_y = v; }
  void setStrideC(unsigned int v) { stride_c = v; }
  void setHeightC(unsigned int v) { height_c = v; }
  void setPlanesC(unsigned int v) { planes_c = v; }
  void setStripes(unsigned int v) { stripes = v; }
  void setBitsPerPixel(unsigned int v) { bits_per_pixel = v; }
  void setBytesPerPixel(unsigned int v) { bits_per_pixel = v * 8; }
private:
  unsigned int stride_y = 0;
  unsigned int height_y = 0;
  unsigned int stride_c = 0;
  unsigned int height_c = 0;
  unsigned int planes_c = 0;
  unsigned int stripes = 0;
  unsigned int bits_per_pixel = 0;
};

class CGPUMEM
{
public:
  CGPUMEM(unsigned int numbytes, bool cached = true);
  ~CGPUMEM();
  void Flush();
  void *m_arm = nullptr; // Pointer to memory mapped on ARM side
  int m_vc_handle = 0;   // Videocore handle of relocatable memory
  int m_vcsm_handle = 0; // Handle for use by VCSM
  unsigned int m_vc = 0;       // Address for use in GPU code
  unsigned int m_numbytes = 0; // Size of memory block
  void *m_opaque = nullptr;
};

class CRBP
{
public:
  CRBP();
  ~CRBP();

  bool Initialize();
  void LogFirmwareVersion();
  void Deinitialize();
  int GetArmMem() { return m_arm_mem; }
  int GetGpuMem() { return m_gpu_mem; }
  bool GetCodecMpg2() { return m_codec_mpg2_enabled; }
  int RaspberryPiVersion() { return g_cpuInfo.getCPUCount() == 1 ? 1 : 2; };
  bool GetCodecWvc1() { return m_codec_wvc1_enabled; }
  void GetDisplaySize(int &width, int &height);
  DISPMANX_DISPLAY_HANDLE_T OpenDisplay(uint32_t device);
  void CloseDisplay(DISPMANX_DISPLAY_HANDLE_T display);
  int GetGUIResolutionLimit() { return m_gui_resolution_limit; }
  // stride can be null for packed output
  unsigned char *CaptureDisplay(int width, int height, int *stride, bool swap_red_blue, bool video_only = true);
  DllOMX *GetDllOMX() { return m_OMX ? m_OMX->GetDll() : NULL; }
  uint32_t LastVsync(int64_t &time);
  uint32_t LastVsync();
  uint32_t WaitVsync(uint32_t target = ~0U);
  void VSyncCallback();
  int GetMBox() { return m_mb; }
  AVRpiZcFrameGeometry GetFrameGeometry(uint32_t encoding, unsigned short video_width, unsigned short video_height);

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
  CCriticalSection m_vsync_lock;
  XbmcThreads::ConditionVariable m_vsync_cond;
  uint32_t m_vsync_count;
  int64_t m_vsync_time;
  class DllLibOMXCore;
  CCriticalSection m_critSection;

  int m_mb;
};

extern CRBP g_RBP;
