/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DVDAudioCodecLiba52.h"
#ifdef USE_LIBA52_DECODER

#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "DVDStreamInfo.h"
#include "utils/log.h"

#ifndef _LINUX
typedef __int16 int16_t;
typedef __int32 int32_t;
#endif

#define HEADER_SIZE 7

static inline int16_t convert(int32_t i)
{
#ifdef LIBA52_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

/**
 * \brief Function to convert the "planar" float format used by liba52
 * into the interleaved int16 format used by us.
 * \param in the input buffer containing the planar samples.
 * \param out the output buffer where the interleaved result is stored.
 * \param channels the total number of channels in the decoded data
 */
static int resample_int16(sample_t * in, int16_t *out, unsigned int channels)
{
    unsigned int i, ch;
    int16_t *p = out;
    for(i = 0; i < 256; ++i)
    {
      for(ch = 0; ch < channels; ++ch)
      {
        *p = convert( ((int32_t*)in)[i + (ch << 8)] );
        ++p;
      }
    }
    return p - out; 
}


CDVDAudioCodecLiba52::CDVDAudioCodecLiba52() : CDVDAudioCodec()
{
  m_pState = NULL;
  SetDefault();
}

CDVDAudioCodecLiba52::~CDVDAudioCodecLiba52()
{
  Dispose();
}

bool CDVDAudioCodecLiba52::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_dll.Load())
    return false;

  SetDefault();

  m_pState = m_dll.a52_init(MM_ACCEL_X86_MMX |MM_ACCEL_X86_MMXEXT);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.a52_samples(m_pState);

  // output will be decided on when
  // we have decoded something
  m_iOutputChannels = 0;

  return true;
}

void CDVDAudioCodecLiba52::Dispose()
{
  if (m_pState) m_dll.a52_free(m_pState);
  m_pState = NULL;
}

void CDVDAudioCodecLiba52::SetupChannels(int flags)
{
  /* These are channel mappings that liba52 outputs */
  static enum PCMChannels channelMaps[14][6] =
  {
    /* Without LFE */
    {/* A52_MONO    */ PCM_FRONT_CENTER                                                                                          },
    {/* A52_STEREO  */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT                                                                         },
    {/* A52_3F      */ PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT                                                      },
    {/* A52_2F1R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_BACK_CENTER                                                      },
    {/* A52_3F1R    */ PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT, PCM_BACK_CENTER                                     },
    {/* A52_2F2R    */ PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT  , PCM_SIDE_RIGHT                                      },
    {/* A52_3F2R    */ PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT, PCM_SIDE_LEFT  , PCM_SIDE_RIGHT                     },
    /* With LFE */
    {/* A52_MONO    */ PCM_LOW_FREQUENCY, PCM_FRONT_CENTER                                                                       },
    {/* A52_STEREO  */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_RIGHT                                                      },
    {/* A52_3F      */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT                                   },
    {/* A52_2F1R    */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_BACK_CENTER                                   },
    {/* A52_3F1R    */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT, PCM_BACK_CENTER                  },
    {/* A52_2F2R    */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_RIGHT  , PCM_SIDE_LEFT  , PCM_SIDE_RIGHT                   },
    {/* A52_3F2R    */ PCM_LOW_FREQUENCY, PCM_FRONT_LEFT  , PCM_FRONT_CENTER , PCM_FRONT_RIGHT, PCM_SIDE_LEFT  , PCM_SIDE_RIGHT  }
  };

  m_iSourceFlags = flags;

  // setup channel map
  int channels = 0;
  int chOffset = (flags & A52_LFE) ? 7 : 0;
  switch (m_iSourceFlags &~ A52_LFE)
  {
    case A52_MONO   : m_pChannelMap = channelMaps[chOffset + 0]; channels = 1; break;
    case A52_CHANNEL:
    case A52_DOLBY  :
    case A52_STEREO : m_pChannelMap = channelMaps[chOffset + 1]; channels = 2; break;
    case A52_3F     : m_pChannelMap = channelMaps[chOffset + 2]; channels = 3; break;
    case A52_2F1R   : m_pChannelMap = channelMaps[chOffset + 3]; channels = 3; break;
    case A52_3F1R   : m_pChannelMap = channelMaps[chOffset + 4]; channels = 4; break;
    case A52_2F2R   : m_pChannelMap = channelMaps[chOffset + 5]; channels = 4; break;
    case A52_3F2R   : m_pChannelMap = channelMaps[chOffset + 6]; channels = 5; break;
    default         : m_pChannelMap = NULL;                                    break;
  }

  if(flags & A52_LFE) ++channels;

  if(m_pChannelMap == NULL)
    CLog::Log(LOGERROR, "CDVDAudioCodecLiba52::SetupChannels - Invalid channel mapping");

  if(m_iSourceChannels > 0 && m_iSourceChannels != channels)
    CLog::Log(LOGINFO, "%s - Number of channels changed in stream from %d to %d, data might be truncated", __FUNCTION__, m_iOutputChannels, channels);

  m_iSourceChannels = channels;
  m_iOutputChannels = m_iSourceChannels;
  m_iOutputFlags    = m_iSourceFlags;

  /* adjust level should always be set, to keep samples in proper range */
  /* after any downmixing has been done */
  m_iOutputFlags |= A52_ADJUST_LEVEL;
}

int CDVDAudioCodecLiba52::ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize)
{
  int flags, len;
  BYTE* orig = data;

  *frame     = NULL;
  *framesize = 0;

  if(m_inputSize == 0 && size > HEADER_SIZE)
  {
    // try to sync directly in packet
    m_iFrameSize = m_dll.a52_syncinfo(data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);

    if(m_iFrameSize > 0)
    {

      if(!m_bFlagsInitialized || m_iSourceFlags != flags)
      {
        SetupChannels(flags);
        m_bFlagsInitialized = true;
      }

      if(size >= m_iFrameSize)
      {
        *frame     = data;
        *framesize = m_iFrameSize;
        return m_iFrameSize;
      }
      else
      {
        m_inputSize = size;
        memcpy(m_inputBuffer, data, m_inputSize);
        return m_inputSize;
      }
    }
  }

  // attempt to fill up to 7 bytes
  if(m_inputSize < HEADER_SIZE)
  {
    len = HEADER_SIZE-m_inputSize;
    if(len > size)
      len = size;
    memcpy(m_inputBuffer+m_inputSize, data, len);
    m_inputSize += len;
    data        += len;
    size        -= len;
  }

  if(m_inputSize < HEADER_SIZE)
    return data - orig;

  // attempt to sync by shifting bytes
  while(true)
  {
    m_iFrameSize = m_dll.a52_syncinfo(m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
    if(m_iFrameSize > 0)
      break;

    if(size == 0)
      return data - orig;

    memmove(m_inputBuffer, m_inputBuffer+1, HEADER_SIZE-1);
    m_inputBuffer[HEADER_SIZE-1] = data[0];
    data++;
    size--;
  }

  if(m_iSourceFlags != flags)
    SetupChannels(flags);

  len = m_iFrameSize-m_inputSize;
  if(len > size)
    len = size;

  memcpy(m_inputBuffer+m_inputSize, data, len);
  m_inputSize += len;
  data        += len;
  size        -= len;

  if(m_inputSize >= m_iFrameSize)
  {
    *frame     = m_inputBuffer;
    *framesize = m_iFrameSize;
    m_inputSize = 0;
  }

  return data - orig;
}

int CDVDAudioCodecLiba52::Decode(BYTE* pData, int iSize)
{
  int len, framesize;
  BYTE* frame;

  m_decodedSize = 0;

  len = ParseFrame(pData, iSize, &frame, &framesize);
  if(!frame)
    return len;

  #if (defined USE_EXTERNAL_LIBA52)
  sample_t level = 1.0f;
  #else
  level_t level = 1.0f;
  #endif
  sample_t bias = 384;
  int     flags = m_iOutputFlags;

  level *= m_Gain;

  m_dll.a52_frame(m_pState, frame, &flags, &level, bias);

  if (!g_advancedSettings.m_audioApplyDrc)
    m_dll.a52_dynrng(m_pState, NULL, NULL);

  for (int i = 0; i < 6; i++)
  {
    if (m_dll.a52_block(m_pState) != 0)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecLiba52::Decode - a52_block failed");
      break;
    }
    m_decodedSize += 2*resample_int16(m_fSamples, (int16_t*)(m_decodedData + m_decodedSize), m_iOutputChannels);
  }
  return len;
}


int CDVDAudioCodecLiba52::GetData(BYTE** dst)
{
  *dst = (BYTE*)m_decodedData;
  return m_decodedSize;
}

void CDVDAudioCodecLiba52::SetDefault()
{
  m_iFrameSize = 0;
  m_bFlagsInitialized = false;
  m_iSourceFlags = 0;
  m_iSourceChannels = 0;
  m_iSourceSampleRate = 0;
  m_iSourceBitrate = 0;
  m_iOutputChannels = 0;
  m_iOutputFlags = 0;
  m_decodedSize = 0;
  m_inputSize = 0;
  m_Gain = 1.0f;
}

void CDVDAudioCodecLiba52::Reset()
{
  if (m_pState) m_dll.a52_free(m_pState);

  SetDefault();

  m_pState = m_dll.a52_init(MM_ACCEL_X86_MMX |MM_ACCEL_X86_MMXEXT); // MMX accel is not really important since liba52 doesnt yet implement this... but whatever...
  m_fSamples = m_dll.a52_samples(m_pState);
}

#endif /* USE_LIBA52_DECODER */
