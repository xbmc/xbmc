/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AEChannelInfo.h"
#include "AEStreamInfo.h"

#define AE_IS_PLANAR(x) ((x) >= AE_FMT_U8P && (x) <= AE_FMT_FLOATP)

/**
 * The audio format structure that fully defines a stream's audio information
 */
struct AEAudioFormat
{
  /**
   * The stream's data format (eg, AE_FMT_S16LE)
   */
  enum AEDataFormat m_dataFormat;

  /**
   * The stream's sample rate (eg, 48000)
   */
  unsigned int m_sampleRate;

  /**
   * The stream's channel layout
   */
  CAEChannelInfo m_channelLayout;

  /**
   * The number of frames per period
   */
  unsigned int m_frames;

  /**
   * The size of one frame in bytes
   */
  unsigned int m_frameSize;

  /**
   * Stream info of raw passthrough
   */
  CAEStreamInfo m_streamInfo;

  AEAudioFormat()
  {
    m_dataFormat = AE_FMT_INVALID;
    m_sampleRate = 0;
    m_frames = 0;
    m_frameSize = 0;
  }

  bool operator==(const AEAudioFormat& fmt) const
  {
    return  m_dataFormat    ==  fmt.m_dataFormat    &&
            m_sampleRate    ==  fmt.m_sampleRate    &&
            m_channelLayout ==  fmt.m_channelLayout &&
            m_frames        ==  fmt.m_frames        &&
            m_frameSize     ==  fmt.m_frameSize     &&
            m_streamInfo    ==  fmt.m_streamInfo;
  }
};

