#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include <vector>

#include "AEChannelInfo.h"
#include "AEStreamInfo.h"

#define AE_IS_PLANAR(x) ((x) >= AE_FMT_U8P && (x) <= AE_FMT_FLOATP)

typedef std::pair<std::string, std::string> AEDevice;
typedef std::vector<AEDevice> AEDeviceList;

/* sound options */
#define AE_SOUND_OFF    0 /* disable sounds */
#define AE_SOUND_IDLE   1 /* only play sounds while no streams are running */
#define AE_SOUND_ALWAYS 2 /* always play sounds */

/* config options */
#define AE_CONFIG_FIXED 1
#define AE_CONFIG_AUTO  2
#define AE_CONFIG_MATCH 3

enum AEQuality
{
  AE_QUALITY_UNKNOWN    = -1, /* Unset, unknown or incorrect quality level */
  AE_QUALITY_DEFAULT    =  0, /* Engine's default quality level */

  /* Basic quality levels */
  AE_QUALITY_LOW        = 20, /* Low quality level */
  AE_QUALITY_MID        = 30, /* Standard quality level */
  AE_QUALITY_HIGH       = 50, /* Best sound processing quality */

  /* Optional quality levels */
  AE_QUALITY_REALLYHIGH = 100, /* Uncompromised optional quality level,
                               usually with unmeasurable and unnoticeable improvement */ 
  AE_QUALITY_GPU        = 101, /* GPU acceleration */
};


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
 
  AEAudioFormat& operator=(const AEAudioFormat& fmt)
  {
    m_dataFormat = fmt.m_dataFormat;
    m_sampleRate = fmt.m_sampleRate;
    m_channelLayout = fmt.m_channelLayout;
    m_frames = fmt.m_frames;
    m_frameSize = fmt.m_frameSize;
    m_streamInfo = fmt.m_streamInfo;

    return *this;
  }
};

