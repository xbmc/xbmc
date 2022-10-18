/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AEAudioFormat.h"
#include "PlatformDefs.h"
#include <math.h>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

// AV sync options
enum AVSync
{
  SYNC_DISCON = 0,
  SYNC_RESAMPLE
};

struct AEDelayStatus
{
  void   SetDelay(double d);
  double GetDelay() const;

  double delay = 0.0;  // delay in sink currently
  double maxcorrection = 0.0; // time correction must not be greater than sink delay
  int64_t tick = 0;  // timestamp when delay was calculated
};

/**
 * @brief lockless consistency guaranteeer
 *
 * Requires write to be a higher priority thread
 *
 * use in writer:
 *   m_locker.enter();
 *   update_stuff();
 *   m_locker.leave();
 *
 * use in reader:
 *   CAESpinLock lock(m_locker);
 *   do {
 *     get_stuff();
 *   } while(lock.retry());
 */

class CAESpinSection
{
public:
  void enter() { m_enter++; }
  void leave() { m_leave = m_enter; }

protected:
  friend class CAESpinLock;
  volatile unsigned int m_enter = 0;
  volatile unsigned int m_leave = 0;
};

class CAESpinLock
{
public:
  explicit CAESpinLock(CAESpinSection& section)
  : m_section(section)
  , m_begin(section.m_enter)
  {}

  bool retry()
  {
    if(m_section.m_enter != m_begin
    || m_section.m_enter != m_section.m_leave)
    {
      m_begin = m_section.m_enter;
      return true;
    }
    else
      return false;
  }

private:
  CAESpinSection& m_section;
  unsigned int    m_begin;
};

class CAEUtil
{
private:

  static float SoftClamp(const float x);

public:
  static CAEChannelInfo          GuessChLayout     (const unsigned int channels);
  static const char*             GetStdChLayoutName(const enum AEStdChLayout layout);
  static unsigned int      DataFormatToBits  (const enum AEDataFormat dataFormat);
  static unsigned int      DataFormatToUsedBits (const enum AEDataFormat dataFormat);
  static unsigned int      DataFormatToDitherBits(const enum AEDataFormat dataFormat);
  static const char*             DataFormatToStr   (const enum AEDataFormat dataFormat);
  static const char* StreamTypeToStr(const enum CAEStreamInfo::DataType dataType);

  /*! \brief convert a volume percentage (as a proportion) to a dB gain
   We assume a dB range of 60dB, i.e. assume that 0% volume corresponds
   to a reduction of 60dB.
   \param value the volume from 0..1
   \return the corresponding gain in dB from -60dB .. 0dB.
   \sa GainToScale
   */
  static inline float PercentToGain(const float value)
  {
    static const float db_range = 60.0f;
    return (value - 1)*db_range;
  }

  /*! \brief convert a dB gain to volume percentage (as a proportion)
   We assume a dB range of 60dB, i.e. assume that 0% volume corresponds
   to a reduction of 60dB.
   \param the corresponding gain in dB from -60dB .. 0dB.
   \return value the volume from 0..1
   \sa ScaleToGain
   */
  static inline float GainToPercent(const float gain)
  {
    static const float db_range = 60.0f;
    return 1+(gain/db_range);
  }

  /*! \brief convert a dB gain to a scale factor for audio manipulation
   Inverts gain = 20 log_10(scale)
   \param dB the gain in decibels.
   \return the scale factor (equivalent to a voltage multiplier).
   \sa PercentToGain
   */
  static inline float GainToScale(const float dB)
  {
    float val = 0.0f;
    // we need to make sure that our lowest db returns plain zero
    if (dB > -60.0f)
      val = pow(10.0f, dB/20);

    // in order to not introduce computing overhead for nearly zero
    // values of dB e.g. -0.01 or -0.001 we clamp to top
    if (val >= 0.99f)
      val = 1.0f;

    return val;
  }

  /*! \brief convert a scale factor to dB gain for audio manipulation
   Inverts GainToScale result
   \param the scale factor (equivalent to a voltage multiplier).
   \return dB the gain in decibels.
   \sa GainToScale
   */
  static inline float ScaleToGain(const float scale)
  {
    return 20*log10(scale);
  }

  #if defined(HAVE_SSE) && defined(__SSE__)
  static void SSEMulArray     (float *data, const float mul, uint32_t count);
  static void SSEMulAddArray  (float *data, float *add, const float mul, uint32_t count);
  #endif
  static void ClampArray(float *data, uint32_t count);

  static bool S16NeedsByteSwap(AEDataFormat in, AEDataFormat out);

  static uint64_t GetAVChannelLayout(const CAEChannelInfo &info);
  static CAEChannelInfo GetAEChannelLayout(uint64_t layout);
  static AVSampleFormat GetAVSampleFormat(AEDataFormat format);
  static uint64_t GetAVChannelMask(enum AEChannel aechannel);
  static enum AVChannel GetAVChannel(enum AEChannel aechannel);
  static int GetAVChannelIndex(enum AEChannel aechannel, uint64_t layout);
};
