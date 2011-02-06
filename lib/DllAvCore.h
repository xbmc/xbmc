#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "DllAvUtil.h"
#include "utils/log.h"

extern "C" {
#ifdef USE_EXTERNAL_FFMPEG
  #ifdef HAVE_LIBAVCORE_AVCORE_H
    #include <libavcore/avcore.h>
  #endif
  #ifdef HAVE_LIBAVCORE_SAMPLEFMT_H
    #include <libavcore/samplefmt.h>
  #endif

  /* Needed for old FFmpeg versions as used below */
  #ifdef HAVE_LIBAVCODEC_AVCODEC_H
    #include <libavcodec/avcodec.h>
  #else
    #include <ffmpeg/avcodec.h>
  #endif
#else
  #include "libavcore/avcore.h"
  #include "libavcore/samplefmt.h"
#endif
}

/* Compatibility for old external FFmpeg versions. */

#ifdef USE_EXTERNAL_FFMPEG

#ifndef LIBAVCORE_VERSION_INT
// API added on: 2010-07-21
#define LIBAVCORE_VERSION_INT 0
#endif

#if LIBAVCORE_VERSION_INT < AV_VERSION_INT(0,10,0)
// API added on: 2010-11-02
#define AVSampleFormat     SampleFormat
#define AV_SAMPLE_FMT_NONE SAMPLE_FMT_NONE
#define AV_SAMPLE_FMT_U8   SAMPLE_FMT_U8
#define AV_SAMPLE_FMT_S16  SAMPLE_FMT_S16
#define AV_SAMPLE_FMT_S32  SAMPLE_FMT_S32
#define AV_SAMPLE_FMT_FLT  SAMPLE_FMT_FLT
#define AV_SAMPLE_FMT_DBL  SAMPLE_FMT_DBL
#endif

#if LIBAVCORE_VERSION_INT < AV_VERSION_INT(0,14,0)
// API added on: 2010-11-21
#define AV_CH_FRONT_LEFT            CH_FRONT_LEFT
#define AV_CH_FRONT_RIGHT           CH_FRONT_RIGHT
#define AV_CH_FRONT_CENTER          CH_FRONT_CENTER
#define AV_CH_LOW_FREQUENCY         CH_LOW_FREQUENCY
#define AV_CH_BACK_LEFT             CH_BACK_LEFT
#define AV_CH_BACK_RIGHT            CH_BACK_RIGHT
#define AV_CH_FRONT_LEFT_OF_CENTER  CH_FRONT_LEFT_OF_CENTER
#define AV_CH_FRONT_RIGHT_OF_CENTER CH_FRONT_RIGHT_OF_CENTER
#define AV_CH_BACK_CENTER           CH_BACK_CENTER
#define AV_CH_SIDE_LEFT             CH_SIDE_LEFT
#define AV_CH_SIDE_RIGHT            CH_SIDE_RIGHT
#define AV_CH_TOP_CENTER            CH_TOP_CENTER
#define AV_CH_TOP_FRONT_LEFT        CH_TOP_FRONT_LEFT
#define AV_CH_TOP_FRONT_CENTER      CH_TOP_FRONT_CENTER
#define AV_CH_TOP_FRONT_RIGHT       CH_TOP_FRONT_RIGHT
#define AV_CH_TOP_BACK_LEFT         CH_TOP_BACK_LEFT
#define AV_CH_TOP_BACK_CENTER       CH_TOP_BACK_CENTER
#define AV_CH_TOP_BACK_RIGHT        CH_TOP_BACK_RIGHT
#define AV_CH_STEREO_LEFT           CH_STEREO_LEFT
#define AV_CH_STEREO_RIGHT          CH_STEREO_RIGHT

#define AV_CH_LAYOUT_NATIVE         CH_LAYOUT_NATIVE

#define AV_CH_LAYOUT_MONO           CH_LAYOUT_MONO
#define AV_CH_LAYOUT_STEREO         CH_LAYOUT_STEREO
#define AV_CH_LAYOUT_2_1            CH_LAYOUT_2_1
#define AV_CH_LAYOUT_SURROUND       CH_LAYOUT_SURROUND
#define AV_CH_LAYOUT_4POINT0        CH_LAYOUT_4POINT0
#define AV_CH_LAYOUT_2_2            CH_LAYOUT_2_2
#define AV_CH_LAYOUT_QUAD           CH_LAYOUT_QUAD
#define AV_CH_LAYOUT_5POINT0        CH_LAYOUT_5POINT0
#define AV_CH_LAYOUT_5POINT1        CH_LAYOUT_5POINT1
#define AV_CH_LAYOUT_5POINT0_BACK   CH_LAYOUT_5POINT0_BACK
#define AV_CH_LAYOUT_5POINT1_BACK   CH_LAYOUT_5POINT1_BACK
#define AV_CH_LAYOUT_7POINT0        CH_LAYOUT_7POINT0
#define AV_CH_LAYOUT_7POINT1        CH_LAYOUT_7POINT1
#define AV_CH_LAYOUT_7POINT1_WIDE   CH_LAYOUT_7POINT1_WIDE
#define AV_CH_LAYOUT_STEREO_DOWNMIX CH_LAYOUT_STEREO_DOWNMIX
#endif

#endif // USE_EXTERNAL_FFMPEG

class DllAvCoreInterface
{
public:
  virtual ~DllAvCoreInterface() {}
  virtual int av_get_bits_per_sample_fmt(enum AVSampleFormat sample_fmt) = 0;
};

#if (defined USE_EXTERNAL_FFMPEG)

// Use direct layer
class DllAvCore : public DllDynamic, DllAvCoreInterface
{
public:
  virtual ~DllAvCore() {}
#if LIBAVCORE_VERSION_INT >= AV_VERSION_INT(0,12,0)
  // API added on: 2010-11-02
  virtual int av_get_bits_per_sample_fmt(enum AVSampleFormat sample_fmt) { return ::av_get_bits_per_sample_fmt(sample_fmt); }
#else
  // from avcodec.h
  virtual int av_get_bits_per_sample_fmt(enum AVSampleFormat sample_fmt) { return ::av_get_bits_per_sample_format(sample_fmt); }
#endif

  // DLL faking.
  virtual bool ResolveExports() { return true; }
  virtual bool Load() {
#if LIBAVCORE_VERSION_INT > 0
    CLog::Log(LOGDEBUG, "DllAvCore: Using libavcore system library");
#endif
    return true;
  }
  virtual void Unload() {}
};

#else

class DllAvCore : public DllDynamic, DllAvCoreInterface
{
  DECLARE_DLL_WRAPPER(DllAvCore, DLL_PATH_LIBAVCORE)

  LOAD_SYMBOLS()

  DEFINE_METHOD1(int, av_get_bits_per_sample_fmt, (enum AVSampleFormat p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(av_get_bits_per_sample_fmt)
  END_METHOD_RESOLVE()

  /* dependency of libavcore */
  DllAvUtil m_dllAvUtil;

public:
  virtual bool Load()
  {
    if (!m_dllAvUtil.Load())
      return false;
    return DllDynamic::Load();
  }
};

#endif

