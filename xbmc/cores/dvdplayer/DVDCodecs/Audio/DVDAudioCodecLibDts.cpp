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
#include "DVDAudioCodecLibDts.h"
#include "DVDStreamInfo.h"

#define HEADER_SIZE 14

static inline int16_t convert(int32_t i)
{
#ifdef LIBDTS_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void CDVDAudioCodecLibDts::convert2s16_multi(convert_t * _f, int16_t * s16, int flags)
{
  register int i;
  register int32_t * f = (int32_t *) _f;

  switch (flags)
  {
    case DTS_MONO:
      for (i = 0; i < 256; i++)
      {
        s16[2*i  ] = convert (f[i]);
        s16[2*i+1] = convert (f[i]);
      }
      break;
    case DTS_CHANNEL:
    case DTS_STEREO:
    case DTS_DOLBY:
      for (i = 0; i < 256; i++)
      {
        s16[2*i  ] = convert (f[i]);
        s16[2*i+1] = convert (f[i+256]);
      }
      break;
    case DTS_3F:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = 0;
        s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i]);
        s16[6*i+4] = 0;
      }
      break;
    case DTS_2F2R:
      for (i = 0; i < 256; i++)
      {
        s16[4*i  ] = convert (f[i]);
        s16[4*i+1] = convert (f[i+256]);
        s16[4*i+2] = convert (f[i+512]);
        s16[4*i+3] = convert (f[i+768]);
      }
      break;
    case DTS_3F2R:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i]);
        s16[6*i+1] = convert (f[i+256]);
        s16[6*i+2] = convert (f[i+512]);
        s16[6*i+3] = convert (f[i+768]);
        s16[6*i+4] = convert (f[i+1024]);
        s16[6*i+5] = 0;
      }
      break;
    case DTS_MONO | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i]);
        s16[6*i+5] = convert (f[i+256]);
      }
      break;
    case DTS_CHANNEL | DTS_LFE:
    case DTS_STEREO | DTS_LFE:
    case DTS_DOLBY | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i]);
        s16[6*i+1] = convert (f[i+256]);
        s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i+512]);
      }
      break;
    case DTS_3F | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = s16[6*i+3] = 0;
        s16[6*i+4] = convert (f[i]);
        s16[6*i+5] = convert (f[i+768]);
      }
      break;
    case DTS_2F2R | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i]);
        s16[6*i+1] = convert (f[i+256]);
        s16[6*i+2] = convert (f[i+512]);
        s16[6*i+3] = convert (f[i+768]);
        s16[6*i+4] = 0;
        s16[6*i+5] = convert (f[i+1024]);
      }
      break;
    case DTS_3F2R | DTS_LFE:
      for (i = 0; i < 256; i++)
      {
        s16[6*i  ] = convert (f[i+256]);
        s16[6*i+1] = convert (f[i+512]);
        s16[6*i+2] = convert (f[i+768]);
        s16[6*i+3] = convert (f[i+1024]);
        s16[6*i+4] = convert (f[i]);
        s16[6*i+5] = convert (f[i+1280]);
      }
      break;
  }
}

CDVDAudioCodecLibDts::CDVDAudioCodecLibDts() : CDVDAudioCodec()
{
  m_pState = NULL;
  SetDefault();
}

CDVDAudioCodecLibDts::~CDVDAudioCodecLibDts()
{
  Dispose();
}

bool CDVDAudioCodecLibDts::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_dll.Load())
    return false;

  SetDefault();
  
  m_pState = m_dll.dts_init(0);
  if (!m_pState)
  {
    Dispose();
    return false;
  }

  m_fSamples = m_dll.dts_samples(m_pState);

  // Output will be decided once we query the stream.
  m_iOutputChannels = 0;
  
  return true;
}

void CDVDAudioCodecLibDts::Dispose()
{
  if (m_pState) m_dll.dts_free(m_pState);
  m_pState = NULL;
}

int CDVDAudioCodecLibDts::GetNrOfChannels(int iFlags)
{
  if(iFlags & DTS_LFE)
    return 6;
  if(iFlags & DTS_CHANNEL)
    return 6;
  else if ((iFlags & DTS_CHANNEL_MASK) == DTS_2F2R)
    return 4;
  else
    return 2;
}

void CDVDAudioCodecLibDts::SetupChannels(int flags)
{
  m_iSourceFlags    = flags;
  m_iSourceChannels = GetNrOfChannels(flags);
  if (g_guiSettings.GetBool("audiooutput.downmixmultichannel"))
    m_iOutputChannels = 2;
  else
    m_iOutputChannels = m_iSourceChannels;

  if (m_iOutputChannels == 1) 
    m_iOutputFlags = DTS_MONO;
  else if (m_iOutputChannels == 2)
    m_iOutputFlags = DTS_STEREO;
  else
    m_iOutputFlags = m_iSourceFlags;

  m_iOutputFlags |= DTS_ADJUST_LEVEL;
}

int CDVDAudioCodecLibDts::ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize)
{
  int flags, len, framelen;
  BYTE* orig = data;

  *frame     = NULL;
  *framesize = 0;

  if(m_inputSize == 0 && size > HEADER_SIZE)
  {
    // try to sync directly in packet
    m_iFrameSize = m_dll.dts_syncinfo(m_pState, data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &framelen);

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
    m_iFrameSize = m_dll.dts_syncinfo(m_pState, m_inputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &framelen);
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

int CDVDAudioCodecLibDts::Decode(BYTE* pData, int iSize)
{
  int len, framesize;
  BYTE* frame;

  m_decodedSize = 0;

  len = ParseFrame(pData, iSize, &frame, &framesize);
  if(!frame)
    return len;
  
  level_t  level = 1.0f;
  sample_t bias  = 384;
  int      flags = m_iOutputFlags;
  
  m_dll.dts_frame(m_pState, frame, &flags, &level, bias);

  //m_dll.dts_dynrng(m_pState, NULL, NULL);
        
  int blocks = m_dll.dts_blocks_num(m_pState);
  for (int i = 0; i < blocks; i++)
  {
    if (m_dll.dts_block(m_pState) != 0)
    {
      CLog::Log(LOGERROR, "CDVDAudioCodecLibDts::Decode : dts_block != 0");
      break;
    }
    convert2s16_multi(m_fSamples, (int16_t*)(m_decodedData + m_decodedSize), flags & (DTS_CHANNEL_MASK | DTS_LFE));
    m_decodedSize += 2 * 256 * m_iOutputChannels;
  }
        
  return len;
}

int CDVDAudioCodecLibDts::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_decodedSize;
}

void CDVDAudioCodecLibDts::SetDefault()
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
}

void CDVDAudioCodecLibDts::Reset()
{
  if (m_pState) m_dll.dts_free(m_pState);

  SetDefault();

  m_pState = m_dll.dts_init(0);
  m_fSamples = m_dll.dts_samples(m_pState);
}

