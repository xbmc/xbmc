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

#include "DVDAudioCodecPassthrough.h"
#if (defined(USE_LIBDTS_DECODER) || defined(USE_LIBA52_DECODER)) && !defined(WIN32)

#include "DVDCodecs/DVDCodecs.h"
#include "DVDStreamInfo.h"
#include "GUISettings.h"
#include "Settings.h"
#include "utils/log.h"

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

//These values are forced to allow spdif out
#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS 2
#define OUT_SAMPLERATE 48000

#define OUT_SAMPLESTOBYTES(a) ((a) * OUT_CHANNELS * (OUT_SAMPLESIZE>>3))

#define HEADER_SIZE 14

/* swab even and uneven data sizes. make sure dst can hold an size aligned to 2 */
static inline int swabdata(char* dst, char* src, int size)
{
  if( size & 0x1 )
  {
    swab(src, dst, size-1);
    dst+=size-1;
    src+=size-1;

    dst[0] = 0x0;
    dst[1] = src[0];
    return size+1;
  }
  else
  {
    swab(src, dst, size);
    return size;
  }

}

CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void)
{
#ifdef USE_LIBA52_DECODER
  m_pStateA52 = NULL;
#endif
#ifdef USE_LIBDTS_DECODER
  m_pStateDTS = NULL;
#endif

  m_iFrameSize=0;

  m_OutputSize = 0;
  m_InputSize  = 0;

  m_OutputBuffer = (BYTE*)_aligned_malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE, 16);
  memset(m_OutputBuffer, 0, AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);

  m_InputBuffer = (BYTE*)_aligned_malloc(4096, 16);
  memset(m_InputBuffer, 0, 4096);
}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  _aligned_free(m_OutputBuffer);
  _aligned_free(m_InputBuffer );
  Dispose();
}

#ifdef USE_LIBDTS_DECODER
int CDVDAudioCodecPassthrough::PaddDTSData( BYTE* pData, int iDataSize, BYTE* pOut)
{
  /* we always output aligned sizes to allow for byteswapping*/
  int iDataSize2 = (iDataSize+1) & ~1;

  pOut[0] = 0x72; pOut[1] = 0xf8; /* iec 61937     */
  pOut[2] = 0x1f; pOut[3] = 0x4e; /*  syncword     */

  switch( m_iSamplesPerFrame )
  {
  case 512:
    pOut[4] = 0x0b;      /* DTS-1 (512-sample bursts) */
    break;
  case 1024:
    pOut[4] = 0x0c;      /* DTS-2 (1024-sample bursts) */
    break;
  case 2048:
    pOut[4] = 0x0d;      /* DTS-3 (2048-sample bursts) */
    break;
  default:
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddDTSData - DTS: %d-sample bursts not supported\n", m_iSamplesPerFrame);
    pOut[4] = 0x00;
    break;
  }

  pOut[5] = 0;                      /* ?? */
  pOut[6] = (iDataSize2 << 3) & 0xFF;
  pOut[7] = (iDataSize2 >> 5) & 0xFF;

  int iOutputSize = OUT_SAMPLESTOBYTES(m_iSamplesPerFrame);

  if ( iDataSize2 > iOutputSize - 8 )
  {
    //Crap frame with more data than we can handle, can be worked around i think
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddDTSData - larger frame than will fit, skipping");
    return 0;
  }

  //Swap byteorder if syncword indicates bigendian
  if( pData[0] == 0x7f || pData[0] == 0x1f )
    swabdata((char*)pOut+8, (char*)pData, iDataSize);
  else
    memcpy((char*)pOut+8, (char*)pData, iDataSize);

  memset(pOut + iDataSize2 + 8, 0, iOutputSize - iDataSize2 - 8);

  return iOutputSize;
}
#endif

#ifdef USE_LIBA52_DECODER
int CDVDAudioCodecPassthrough::PaddAC3Data( BYTE* pData, int iDataSize, BYTE* pOut)
{
  /* we always output aligned sizes to allow for byteswapping*/
  int iDataSize2 = (iDataSize+1) & ~1;

  //Setup ac3 header
  pOut[0] = 0x72;
  pOut[1] = 0xF8;
  pOut[2] = 0x1F;
  pOut[3] = 0x4E;
  pOut[4] = 0x01; //(length) ? data_type : 0; /* & 0x1F; */
  pOut[5] = 0x00;
  pOut[6] = (iDataSize2 << 3) & 0xFF;
  pOut[7] = (iDataSize2 >> 5) & 0xFF;

  int iOutputSize = OUT_SAMPLESTOBYTES(m_iSamplesPerFrame);

  if ( iDataSize2 > iOutputSize - 8 )
  {
    //Crap frame with more data than we can handle, can be worked around i think
    CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::PaddAC3Data - larger frame than will fit, skipping");
    return 0;
  }

  //Swap byteorder
  swabdata((char*)pOut+8, (char*)pData, iDataSize);
  memset(pOut + iDataSize2 + 8, 0, iOutputSize - iDataSize2 - 8);
  return iOutputSize;
}
#endif

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsAC3Out = false;
  bool bSupportsDTSOut = false;
  int audioMode = g_guiSettings.GetInt("audiooutput.mode");

  // TODO - move this stuff somewhere else
  if (AUDIO_IS_BITSTREAM(audioMode))
  {
    bSupportsAC3Out = g_guiSettings.GetBool("audiooutput.ac3passthrough");
    bSupportsDTSOut = g_guiSettings.GetBool("audiooutput.dtspassthrough");
  }

  //Samplerate cannot be checked here as we don't know it at this point in time.
  //We should probably have a way to try to decode data so that we know what samplerate it is.
  if ((hints.codec == CODEC_ID_AC3 && bSupportsAC3Out)
   || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut))
  {

    // TODO - this is only valid for video files, and should be moved somewhere else
    if( hints.channels == 2 && g_settings.m_currentVideoSettings.m_OutputToAllSpeakers )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to video OTAS");
      return false;
    }

    // TODO - some soundcards do support other sample rates, but they are quite uncommon
    if( hints.samplerate > 0 && hints.samplerate != 48000 )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to sample rate not being 48000");
      return false;
    }

    /* try AC3/DTS decoders first */
    m_Codec = hints.codec;
#ifdef USE_LIBA52_DECODER
    if(m_Codec == CODEC_ID_AC3)
    {
      if (!m_dllA52.Load())
        return false;
      m_pStateA52 = m_dllA52.a52_init(0);
      if(m_pStateA52 == NULL)
        return false;
      m_iSamplesPerFrame = 6*256;
      return true;
    }
#endif
#ifdef USE_LIBDTS_DECODER
    if(m_Codec == CODEC_ID_DTS)
    {
      if (!m_dllDTS.Load())
        return false;
      m_pStateDTS = m_dllDTS.dts_init(0);
      if(m_pStateDTS == NULL)
        return false;
      return true;
    }
#endif
  }

  return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
#ifdef USE_LIBA52_DECODER
  if( m_pStateA52 )
  {
    m_dllA52.a52_free(m_pStateA52);
    m_pStateA52 = NULL;
  }
#endif
#ifdef USE_LIBDTS_DECODER
  if( m_pStateDTS )
  {
    m_dllDTS.dts_free(m_pStateDTS);
    m_pStateDTS = NULL;
  }
#endif
}

int CDVDAudioCodecPassthrough::ParseFrame(BYTE* data, int size, BYTE** frame, int* framesize)
{
  int flags, len;
  BYTE* orig = data;

  *frame     = NULL;
  *framesize = 0;

  if(m_InputSize == 0 && size > HEADER_SIZE)
  {
    // try to sync directly in packet
#ifdef USE_LIBA52_DECODER
    if(m_Codec == CODEC_ID_AC3)
      m_iFrameSize = m_dllA52.a52_syncinfo(data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
#endif
#ifdef USE_LIBDTS_DECODER
    if(m_Codec == CODEC_ID_DTS)
      m_iFrameSize = m_dllDTS.dts_syncinfo(m_pStateDTS, data, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame);
#endif

    if(m_iFrameSize > 0)
    {

      if(m_iSourceFlags != flags)
      {
        m_iSourceFlags = flags;
        CLog::Log(LOGDEBUG, "%s - source flags changed flags:%x sr:%d br:%d", __FUNCTION__, m_iSourceFlags, m_iSourceSampleRate, m_iSourceBitrate);
      }

      if(size >= m_iFrameSize)
      {
        *frame     = data;
        *framesize = m_iFrameSize;
        return m_iFrameSize;
      }
      else
      {
        m_InputSize = size;
        memcpy(m_InputBuffer, data, m_InputSize);
        return m_InputSize;
      }
    }
  }

  // attempt to fill up to 7 bytes
  if(m_InputSize < HEADER_SIZE)
  {
    len = HEADER_SIZE-m_InputSize;
    if(len > size)
      len = size;
    memcpy(m_InputBuffer+m_InputSize, data, len);
    m_InputSize += len;
    data        += len;
    size        -= len;
  }

  if(m_InputSize < HEADER_SIZE)
    return data - orig;

  // attempt to sync by shifting bytes
  while(true)
  {
#ifdef USE_LIBA52_DECODER
    if(m_Codec == CODEC_ID_AC3)
      m_iFrameSize = m_dllA52.a52_syncinfo(m_InputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate);
#endif
#ifdef USE_LIBDTS_DECODER
    if(m_Codec == CODEC_ID_DTS)
      m_iFrameSize = m_dllDTS.dts_syncinfo(m_pStateDTS, m_InputBuffer, &flags, &m_iSourceSampleRate, &m_iSourceBitrate, &m_iSamplesPerFrame);
#endif
    if(m_iFrameSize > 0)
      break;

    if(size == 0)
      return data - orig;

    memmove(m_InputBuffer, m_InputBuffer+1, HEADER_SIZE-1);
    m_InputBuffer[HEADER_SIZE-1] = data[0];
    data++;
    size--;
  }

  if(m_iSourceFlags != flags)
  {
    m_iSourceFlags = flags;
    CLog::Log(LOGDEBUG, "%s - source flags changed flags:%x sr:%d br:%d", __FUNCTION__, m_iSourceFlags, m_iSourceSampleRate, m_iSourceBitrate);
  }

  len = m_iFrameSize-m_InputSize;
  if(len > size)
    len = size;

  memcpy(m_InputBuffer+m_InputSize, data, len);
  m_InputSize += len;
  data        += len;
  size        -= len;

  if(m_InputSize >= m_iFrameSize)
  {
    *frame     = m_InputBuffer;
    *framesize = m_iFrameSize;
    m_InputSize = 0;
  }

  return data - orig;
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  int len, framesize;
  BYTE* frame;
  m_OutputSize = 0;

  len = ParseFrame(pData, iSize, &frame, &framesize);
  if(!frame)
    return len;

#ifdef USE_LIBA52_DECODER
  if(m_Codec == CODEC_ID_AC3)
  {
    m_OutputSize = PaddAC3Data(frame, framesize, m_OutputBuffer);
    return len;
  }
#endif
#ifdef USE_LIBDTS_DECODER
  if(m_Codec == CODEC_ID_DTS)
  {
    m_OutputSize = PaddDTSData(frame, framesize, m_OutputBuffer);
    return len;
  }
#endif
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  int size;
  if(m_OutputSize)
  {
    *dst = m_OutputBuffer;
    size = m_OutputSize;

    m_OutputSize = 0;
    return size;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_InputSize = 0;
  m_OutputSize = 0;
  m_Synced = false;
}

int CDVDAudioCodecPassthrough::GetChannels()
{
  //Can't return correct channels here as this is used to keep sync.
  //should probably have some other way to find out this
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  return m_iSourceSampleRate;
}

int CDVDAudioCodecPassthrough::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

#endif
