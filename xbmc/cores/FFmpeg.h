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


#include "utils/CPUInfo.h"

extern "C" {
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libavfilter/avfilter.h"
#include "libpostproc/postprocess.h"
}

inline int SwScaleCPUFlags()
{
  unsigned int cpuFeatures = g_cpuInfo.GetCPUFeatures();
  int flags = 0;

  if (cpuFeatures & CPU_FEATURE_MMX)
    flags |= SWS_CPU_CAPS_MMX;
  if (cpuFeatures & CPU_FEATURE_MMX2)
    flags |= SWS_CPU_CAPS_MMX2;
  if (cpuFeatures & CPU_FEATURE_3DNOW)
    flags |= SWS_CPU_CAPS_3DNOW;
  if (cpuFeatures & CPU_FEATURE_ALTIVEC)
    flags |= SWS_CPU_CAPS_ALTIVEC;

  return flags;
}

inline int PPCPUFlags()
{
  unsigned int cpuFeatures = g_cpuInfo.GetCPUFeatures();
  int flags = 0;

  if (cpuFeatures & CPU_FEATURE_MMX)
    flags |= PP_CPU_CAPS_MMX;
  if (cpuFeatures & CPU_FEATURE_MMX2)
    flags |= PP_CPU_CAPS_MMX2;
  if (cpuFeatures & CPU_FEATURE_3DNOW)
    flags |= PP_CPU_CAPS_3DNOW;
  if (cpuFeatures & CPU_FEATURE_ALTIVEC)
    flags |= PP_CPU_CAPS_ALTIVEC;

  return flags;
}

// callback used for locking
int ffmpeg_lockmgr_cb(void **mutex, enum AVLockOp operation);

// callback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);
void ff_flush_avutil_log_buffers(void);

