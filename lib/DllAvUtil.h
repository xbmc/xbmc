#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include "DynamicDll.h"
#include "utils/log.h"

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable:4244)
#endif

extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVUTIL_AVUTIL_H)
    #include <libavutil/avutil.h>
    // for av_get_default_channel_layout
    #include <libavutil/audioconvert.h>
    #include <libavutil/crc.h>
    #include <libavutil/fifo.h>
    // for enum AVSampleFormat
    #include <libavutil/samplefmt.h>
    // for LIBAVCODEC_VERSION_INT:
    #include <libavcodec/avcodec.h>
  #elif (defined HAVE_FFMPEG_AVUTIL_H)
    #include <ffmpeg/avutil.h>
    // for av_get_default_channel_layout
    #include <ffmpeg/audioconvert.h>
    #include <ffmpeg/crc.h>
    #include <ffmpeg/fifo.h>
    // for enum AVSampleFormat
    #include <ffmpeg/samplefmt.h>
    // for LIBAVCODEC_VERSION_INT:
    #include <ffmpeg/avcodec.h>
  #endif
  #if defined(HAVE_LIBAVUTIL_OPT_H)
    #include <libavutil/opt.h>
  #elif defined(HAVE_LIBAVCODEC_AVCODEC_H)
    #include <libavcodec/opt.h>
  #else
    #include <ffmpeg/opt.h>
  #endif
  #if defined(HAVE_LIBAVUTIL_MEM_H)
    #include <libavutil/mem.h>
  #else
    #include <ffmpeg/mem.h>
  #endif
  #if (defined HAVE_LIBAVUTIL_MATHEMATICS_H)
    #include <libavutil/mathematics.h>
  #endif
#else
  #include "libavutil/avutil.h"
  //for av_get_default_channel_layout
  #include "libavutil/audioconvert.h"
  #include "libavutil/crc.h"
  #include "libavutil/opt.h"
  #include "libavutil/mem.h"
  #include "libavutil/fifo.h"
  // for enum AVSampleFormat
  #include "libavutil/samplefmt.h"
#endif
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

// calback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);

// callback used for locking
int ffmpeg_lockmgr_cb(void **mutex, enum AVLockOp operation);

#ifdef TARGET_WINDOWS
#pragma comment(lib, "avutil.lib")
#endif
