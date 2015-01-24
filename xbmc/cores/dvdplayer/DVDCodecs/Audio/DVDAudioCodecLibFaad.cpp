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

#include "DVDAudioCodecLibFaad.h"
#ifdef USE_LIBFAAD_DECODER

#include "DVDStreamInfo.h"
#include "GUISettings.h"
#include "utils/log.h"

CDVDAudioCodecLibFaad::CDVDAudioCodecLibFaad() : CDVDAudioCodec()
{
  m_bInitializedDecoder = false;

  m_pHandle = NULL;
}

CDVDAudioCodecLibFaad::~CDVDAudioCodecLibFaad()
{
  Dispose();
}

bool CDVDAudioCodecLibFaad::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  // for safety
  if (m_pHandle) Dispose();

  if (!m_dll.Load())
    return false;

  memset(&m_frameInfo, 0, sizeof(m_frameInfo));

  if (!OpenDecoder() )
    return false;

  m_bRawAACStream = true;;

  if( hints.extrasize )
  {
    m_bRawAACStream = false;

    unsigned long samplerate;
    unsigned char channels;

    int res = m_dll.faacDecInit2(m_pHandle, (unsigned char*)hints.extradata, hints.extrasize, &samplerate, &channels);
    if (res < 0)
      return false;

    m_iSourceSampleRate = samplerate;
    m_iSourceChannels = channels;
    m_iSourceBitrate = 0;

    m_bInitializedDecoder = true;
  }
  return true;
}

void CDVDAudioCodecLibFaad::Dispose()
{
  CloseDecoder();
}

bool CDVDAudioCodecLibFaad::SyncStream()
{
  BYTE* p = m_InputBuffer;

  while (m_InputBufferSize > 4)
  {
    // Check if an ADIF or ADTS header is present
    if (((p[0] == 'A') && (p[1] == 'D') && (p[2] == 'I') && (p[3] == 'F')) ||
        ((p[1] | p[0] << 8) & 0xfff0) == 0xfff0)
    {
      // sync found, update our buffer if needed
      if (p != m_InputBuffer)
      {
        CLog::Log(LOGINFO, "CDVDAudioCodecLibFaad::SyncStream(), stream synced at offset %d", (int)(p - m_InputBuffer));
        memmove(m_InputBuffer, p, m_InputBufferSize);
      }
      return true;
    }
    p++;
    m_InputBufferSize--;
  }

  // no sync found
  CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad::SyncStream(), no sync found (ADIF or ADTS header) in stream");
  return false;
}

enum PCMChannels* CDVDAudioCodecLibFaad::GetChannelMap()
{
  int index = 0;
  for(int i = 0; i < m_iSourceChannels; ++i)
    switch(m_frameInfo.channel_position[i])
    {
      case FRONT_CHANNEL_CENTER: m_pChannelMap[index++] = PCM_FRONT_CENTER ; break;
      case FRONT_CHANNEL_LEFT  : m_pChannelMap[index++] = PCM_FRONT_LEFT   ; break;
      case FRONT_CHANNEL_RIGHT : m_pChannelMap[index++] = PCM_FRONT_RIGHT  ; break;
      case SIDE_CHANNEL_LEFT   : m_pChannelMap[index++] = PCM_SIDE_LEFT    ; break;
      case SIDE_CHANNEL_RIGHT  : m_pChannelMap[index++] = PCM_SIDE_RIGHT   ; break;
      case BACK_CHANNEL_LEFT   : m_pChannelMap[index++] = PCM_BACK_LEFT    ; break;
      case BACK_CHANNEL_RIGHT  : m_pChannelMap[index++] = PCM_BACK_RIGHT   ; break;
      case BACK_CHANNEL_CENTER : m_pChannelMap[index++] = PCM_BACK_CENTER  ; break;
      case LFE_CHANNEL         : m_pChannelMap[index++] = PCM_LOW_FREQUENCY; break;
    }

  if (index < m_iSourceChannels)
    return NULL;

  assert(index == m_iSourceChannels);
  return m_pChannelMap;
}

int CDVDAudioCodecLibFaad::Decode(BYTE* pData, int iSize)
{
  m_DecodedDataSize = 0;

  if (!m_pHandle)
    return -1;

  int iBytesToCopy = std::min(iSize, LIBFAAD_INPUT_SIZE - m_InputBufferSize);
  memcpy(m_InputBuffer + m_InputBufferSize, pData, iBytesToCopy);
  m_InputBufferSize += iBytesToCopy;

  // if the caller does not supply enough data, return
  if (m_InputBufferSize < FAAD_MIN_STREAMSIZE)
    return iBytesToCopy;

  if(m_bRawAACStream)
  {
    // attempt to sync stream
    if (!SyncStream())
      return iBytesToCopy;

    // initialize decoder if needed
    if (!m_bInitializedDecoder)
    {
      unsigned long samplerate;
      unsigned char channels;

      int res = m_dll.faacDecInit(m_pHandle, m_InputBuffer, m_InputBufferSize, &samplerate, &channels);
      if(res < 0)
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecLibFaad() : unable to init faad");
        m_InputBufferSize = 0;

        // faac leeks when faacDecInit is called multiple times on same handle
        CloseDecoder();
        if(!OpenDecoder())
          return -1;
      }
      else
      {
        m_iSourceSampleRate = samplerate;
        m_iSourceChannels = channels;
        m_iSourceBitrate = 0;

        m_bInitializedDecoder = true;
      }
    }
  }

  // if we haven't succeded in initing now, keep going
  if (!m_bInitializedDecoder)
    return iBytesToCopy;

  m_DecodedData = (short*)m_dll.faacDecDecode(m_pHandle, &m_frameInfo, m_InputBuffer, m_InputBufferSize);
  m_DecodedDataSize = m_frameInfo.samples * sizeof(short);

  if (m_frameInfo.error)
  {
    char* strError = m_dll.faacDecGetErrorMessage(m_frameInfo.error);
    m_dll.faacDecPostSeekReset(m_pHandle, 0);
    CLog::Log(LOGERROR, "CDVDAudioCodecLibFaad() : %s", strError);
    m_InputBufferSize = 0;
    return iBytesToCopy;
  }

  // we set this info again, it could be this info changed
  m_iSourceSampleRate = m_frameInfo.samplerate;
  m_iSourceChannels = m_frameInfo.channels;
  m_iSourceBitrate = 0;

  // move remaining data along
  m_InputBufferSize -= m_frameInfo.bytesconsumed;
  memcpy(m_InputBuffer, m_InputBuffer+m_frameInfo.bytesconsumed, m_InputBufferSize);

  return iBytesToCopy;
}

int CDVDAudioCodecLibFaad::GetData(BYTE** dst)
{
  *dst = (BYTE*)m_DecodedData;
  return m_DecodedDataSize;
}

void CDVDAudioCodecLibFaad::Reset()
{
  if (m_pHandle)
    m_dll.faacDecPostSeekReset(m_pHandle, 0);
}

void CDVDAudioCodecLibFaad::CloseDecoder()
{
  if (m_pHandle)
  {
    m_dll.faacDecClose(m_pHandle);
    m_pHandle = NULL;
  }
}

bool CDVDAudioCodecLibFaad::OpenDecoder()
{
  if (m_pHandle)
  {
    CLog::Log(LOGWARNING, "CDVDAudioCodecLibFaad : Decoder already opened");
    return false;
  }

  m_bInitializedDecoder = false;

  m_InputBufferSize = 0;
  m_DecodedDataSize = 0;

  m_iSourceSampleRate = 0;
  m_iSourceChannels = 0;
  m_iSourceBitrate = 0;

  m_pHandle = m_dll.faacDecOpen();

  if (m_pHandle)
  {
    faacDecConfigurationPtr pConfiguration;
    pConfiguration = m_dll.faacDecGetCurrentConfiguration(m_pHandle);

    // modify some stuff here
    pConfiguration->outputFormat = FAAD_FMT_16BIT; // already default
    pConfiguration->downMatrix   = 0;

    m_dll.faacDecSetConfiguration(m_pHandle, pConfiguration);

    return true;
  }

  return false;
}

#endif
