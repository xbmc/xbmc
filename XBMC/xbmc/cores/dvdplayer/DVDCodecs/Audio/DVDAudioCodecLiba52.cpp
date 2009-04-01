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
 
#include "stdafx.h"
#include "Settings.h"
#include "DVDAudioCodecLiba52.h"
#include "DVDStreamInfo.h"

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
 */
static int resample_int16(sample_t * in, int16_t *out, int32_t channel_map)
{
    unsigned long i;
    int16_t *p = out;
    for (i = 0; i != 256; i++) {
	unsigned long map = channel_map;
	do {
	    unsigned long ch = map & 15;
	    if (ch == 15)
		*p = 0;
	    else
		*p = convert( ((int32_t*)in)[i + ((ch-1)<<8)] );
	    p++;
	} while ((map >>= 4));
    }
    return (int16_t*) p - out;
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
/* Internal AC3 ordering for different configs
 * A52_CHANNEL: 1+1 2 Ch1, Ch2
 * A52_MONO   : 1/0 1 C
 * A52_STEREO : 2/0 2 L, R
 * A52_3F     : 3/0 3 L, C, R
 * A52_2F1R   : 2/1 3 L, R, S
 * A52_3F1R   : 3/1 4 L, C, R, S
 * A52_2F2R   : 2/2 4 L, R, SL, SR
 * A52_3F2R   : 3/2 5 L, C, R, SL, SR
*/

  m_iSourceFlags = flags;
  // setup channel map for how to translate to linear format
  // standard windows format
  if(m_iSourceFlags & A52_LFE)
  {
    switch (m_iSourceFlags&~A52_LFE) {
      case A52_MONO   : m_iOutputMapping = 0x12ffff; break;
      case A52_CHANNEL:
      case A52_STEREO :
      case A52_DOLBY  : m_iOutputMapping = 0x1fff32; break;
      case A52_2F1R   : m_iOutputMapping = 0x1f5542; break;
      case A52_2F2R   : m_iOutputMapping = 0x1f5432; break;
      case A52_3F     : m_iOutputMapping = 0x13ff42; break;
      case A52_3F1R   : m_iOutputMapping = 0x135542; break;
      case A52_3F2R   : m_iOutputMapping = 0x136542; break;
      default         : m_iOutputMapping = 0x000000; break;
    }
  }
  else
  {
    switch (m_iSourceFlags) {
      case A52_MONO   : m_iOutputMapping =     0x1; break;
      case A52_CHANNEL:
      case A52_STEREO :
      case A52_DOLBY  : m_iOutputMapping =    0x21; break;
      case A52_2F1R   : m_iOutputMapping =  0x2231; break;
      case A52_2F2R   : m_iOutputMapping =  0x4321; break;
      case A52_3F     : m_iOutputMapping = 0x2ff31; break;
      case A52_3F1R   : m_iOutputMapping = 0x24431; break;
      case A52_3F2R   : m_iOutputMapping = 0x25431; break;
      default         : m_iOutputMapping =     0x0; break;
    }
  }

  if(m_iOutputMapping == 0x0)
    CLog::Log(LOGERROR, "CDVDAudioCodecLiba52::SetupChannels - Invalid channel mapping");

  int channels = 0;
  unsigned int m = m_iOutputMapping<<4;
  while(m>>=4) channels++;  

  // xbox can't handle these
  if(channels == 5 || channels == 3)
    channels = 6;

  if(m_iSourceChannels > 0 && m_iSourceChannels != channels)
    CLog::Log(LOGINFO, "%s - Number of channels changed in stream from %d to %d, data might be truncated", __FUNCTION__, m_iOutputChannels, channels);

  m_iSourceChannels = channels;

  // make sure map contains enough channels
  for(int i=0;i<m_iSourceChannels;i++)
  {
    if((m_iOutputMapping & (0xf<<(i*4))) == 0)
      m_iOutputMapping |= 0xf<<(i*4);
  }
  // and nothing more
  m_iOutputMapping &= ~(0xffffffff<<(m_iSourceChannels*4));


  m_iOutputChannels = m_iSourceChannels;
  m_iOutputFlags    = m_iSourceFlags;

  // If we can't support multichannel output downmix
  if (g_guiSettings.GetBool("audiooutput.downmixmultichannel"))
  {
    m_iOutputChannels = 2;
    m_iOutputMapping = 0x21;
    m_iOutputFlags = A52_STEREO;
    if (m_iSourceChannels > 2)
      m_Gain = pow(2,g_advancedSettings.m_ac3Gain/6.0f); // Hack for downmix attenuation
  }

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

      if(m_iSourceFlags != flags)
        SetupChannels(flags);

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

  level_t level = 1.0f;
  sample_t bias = 384;
  int     flags = m_iOutputFlags;

  level *= m_Gain;

  m_dll.a52_frame(m_pState, frame, &flags, &level, bias);

  //m_dll.a52_dynrng(m_pState, NULL, NULL);
  
  for (int i = 0; i < 6; i++)
  {
    if (m_dll.a52_block(m_pState) != 0)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecLiba52::Decode - a52_block failed");
      break;
    }
    m_decodedSize += 2*resample_int16(m_fSamples, (int16_t*)(m_decodedData + m_decodedSize), m_iOutputMapping);
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

