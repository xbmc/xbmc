
#include "stdafx.h"
#include "DVDAudioCodecPassthrough.h"
#include "../DVDCodecs.h"
#include "../../DVDStreamInfo.h"
#include "../../../../XBAudioConfig.h"

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

//These values are forced to allow spdif out
#define OUT_SAMPLESIZE 16
#define OUT_CHANNELS 2
#define OUT_SAMPLERATE 48000

#define OUT_SAMPLESTOBYTES(a) ((a) * OUT_CHANNELS * (OUT_SAMPLESIZE>>3))

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
  m_pPassBuffer = NULL;
  m_pStateA52 = NULL;
  m_pStateDTS = NULL;
  m_iPassBufferLen = 0;

  m_iDataFrameAlloced = 6144*2;
  m_pDataFrame = (BYTE*)malloc(m_iDataFrameAlloced);
  m_iDataFrameLen = 0;
  m_iOffset=0;

  m_iFrameSize=0;
  m_iType = ENS_UNKNOWN;

  m_iPassBufferAlloced = 0;
  m_pPassBuffer = NULL;
  m_iPassBufferLen = 0;

}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}


bool CDVDAudioCodecPassthrough::SyncDTSHeader(BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize)
{
  int i = 0, iLen = 0;
  int iFlags, iSampleRate, iBitRate, iFrameLenght;

  if( !m_pStateDTS ) return false;

  while (i <= (iDataSize - 10))
  {
    iLen = m_dllDTS.dts_syncinfo(m_pStateDTS, pData, &iFlags, &iSampleRate, &iBitRate, &iFrameLenght);
    if (iLen > 0)
    {

      if( iSampleRate != OUT_SAMPLERATE )
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::SyncDTSHeader - unsupported samplerate %d, skipping", iSampleRate);
        return false;
      }

      m_iSamplesPerFrame = iFrameLenght;
      m_iSampleRate = iSampleRate;

      (*iFrameSize) = iLen;
      (*iOffset) = i;
      return true;
    }

    // no sync found, shift one byte
    i++;
    pData++;
    iDataSize--;
  } 
  return false;
}

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

bool CDVDAudioCodecPassthrough::SyncAC3Header(BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize )
{
  //static const int ac3Channels[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

  int i = 0, iLen = 0;
  int iFlags, iSampleRate, iBitRate;

  if( !m_pStateA52 ) return false;

  while (i <= (iDataSize - 7))
  {
    iLen = m_dllA52.a52_syncinfo(pData, &iFlags, &iSampleRate, &iBitRate);
    if (iLen > 0)
    {
      if( iSampleRate != OUT_SAMPLERATE )
      {
        CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough::SyncAC3Header - unsupported samplerate %d, skipping", iSampleRate);
        return false;
      }

      m_iSamplesPerFrame = 6*256;        
      m_iSampleRate = iSampleRate;

      (*iFrameSize) = iLen;
      (*iOffset) = i;
      return true;
    }

    // no sync found, shift one byte
    i++;
    pData++;
    iDataSize--;
  } 
  return false;
}

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

bool CDVDAudioCodecPassthrough::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  bool bSupportsAC3Out = false,  bSupportsDTSOut = false;


  // TODO - move this stuff somewhere else
  if (g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL)
  {
    bSupportsAC3Out = g_audioConfig.GetAC3Enabled();
    bSupportsDTSOut = g_audioConfig.GetDTSEnabled();
  }  

  //Samplerate cannot be checked here as we don't know it at this point in time. 
  //We should probably have a way to try to decode data so that we know what samplerate it is.
  if ((hints.codec == CODEC_ID_AC3 && bSupportsAC3Out) || (hints.codec == CODEC_ID_DTS && bSupportsDTSOut))
  {

    // TODO - this is only valid for video files, and should be moved somewhere else
    if( hints.channels == 2 && g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers )
    {
      CLog::Log(LOGINFO, "CDVDAudioCodecPassthrough::Open - disabled passthrough due to video OTAS");
      return false;
    }


    //We load both libDTS and A54 independent on what codec it was. Some streams are marked wrong
    if (m_dllDTS.Load())
    {
      m_pStateDTS = m_dllDTS.dts_init(0);
    }

    if (m_dllA52.Load())
    {
      m_pStateA52 = m_dllA52.a52_init(0);
    }
    return true;    
  }
  else
    return false;
}

void CDVDAudioCodecPassthrough::Dispose()
{
  if( m_pDataFrame )
  {
    free(m_pDataFrame);
    m_pDataFrame = NULL;
    m_iDataFrameAlloced = 0;
    m_iDataFrameLen = 0;
  }
  if( m_pStateA52 )
  {
    m_dllA52.a52_free(m_pStateA52);
    m_pStateA52 = NULL;
  }
  if( m_pStateDTS )
  {
    m_dllDTS.dts_free(m_pStateDTS);
    m_pStateDTS = NULL;
  }
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{  

  //If there is more to pass, do so first
  if( m_iPassBufferLen > 0) return 0;

  //Make sure we have enough room
  if( iSize + m_iDataFrameLen > m_iDataFrameAlloced )
  {
    m_pDataFrame = (BYTE*)realloc(m_pDataFrame, iSize + m_iDataFrameLen);
    if ( !m_pDataFrame ) CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough: Unable to allocate buffer, crash imminent");
    m_iDataFrameAlloced = iSize + m_iDataFrameLen;
  }

  //Copy data to our buffer
  memcpy((char*)m_pDataFrame+m_iDataFrameLen, pData, iSize);
  m_iDataFrameLen+=iSize;

  //Need atleast 10 bytes to sync
  if( m_iDataFrameLen < 10 ) return iSize;

  if( m_iFrameSize == 0)
  {
    m_iOffset = 0;

    if( SyncDTSHeader(m_pDataFrame, m_iDataFrameLen, &m_iOffset, &m_iFrameSize) )
    {
      if(m_iType != ENS_DTS)
      {
        CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough: Synced to DTS frame");
        m_iType = ENS_DTS;
      }
    }
    // Check for AC3 sync only if no DTS sync was found. AC3 seem to sync sometimes even thou it's a DTS stream
    // i would suppose a dts stream can contain data that looks like the AC3 syncword.
    else if( SyncAC3Header(m_pDataFrame, m_iDataFrameLen, &m_iOffset, &m_iFrameSize) )
    {
      if(m_iType != ENS_AC3)
      {
        CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough: Synced to AC3 frame");
        m_iType = ENS_AC3;
      }
    }
  }

  if (m_iFrameSize == 0)
  {
    //No sync found in current data, dump everything except the last 9 bytes (dts needs 10 to sync)
    m_iDataFrameLen=0;
    return MAX(iSize-9, 0);
  }

  //Check our data to see if we have enough to complete a frame
  if (m_iDataFrameLen >= m_iFrameSize + m_iOffset && m_iFrameSize > 0)
  {
    if( m_iPassBufferAlloced < OUT_SAMPLESTOBYTES(m_iSamplesPerFrame) )
    {
      m_iPassBufferAlloced = OUT_SAMPLESTOBYTES(m_iSamplesPerFrame);
      m_pPassBuffer = (BYTE*)realloc(m_pPassBuffer, m_iPassBufferAlloced);
    }

    if(m_iType == ENS_DTS)
    {
      m_iPassBufferLen = PaddDTSData(m_pDataFrame + m_iOffset, m_iFrameSize, m_pPassBuffer);
    }
    else if(m_iType == ENS_AC3)
    {
      m_iPassBufferLen = PaddAC3Data(m_pDataFrame + m_iOffset, m_iFrameSize, m_pPassBuffer);
    }    
    //Calc how much we use of size to complete frame
    int ret = m_iFrameSize + m_iOffset - (m_iDataFrameLen-iSize); 

    //Discard current data
    m_iDataFrameLen = 0;
    m_iFrameSize=0;
    m_iOffset=0;
    return ret;
  }
  else
  {
    //Not enough data to complete a frame, grab everything decoder gave me so it get's some more
    return iSize;
  }  

  CLog::Log(LOGWARNING, "CDVDAudioCodecPassthrough::Decode - Error: Unable to sync package");
  return -1;
}

int CDVDAudioCodecPassthrough::GetData(BYTE** dst)
{
  if(m_iPassBufferLen)
  {
    *dst = m_pPassBuffer;
    int iSize = m_iPassBufferLen;
    m_iPassBufferLen = 0;
    return iSize;
  }
  else
    return 0;
}

void CDVDAudioCodecPassthrough::Reset()
{
  m_iPassBufferLen = 0;
  m_iDataFrameLen = 0;
  m_iType = ENS_UNKNOWN;
}

int CDVDAudioCodecPassthrough::GetChannels()
{
  //Can't return correct channels here as this is used to keep sync.
  //should probably have some other way to find out this
  return OUT_CHANNELS;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  return OUT_SAMPLERATE;
}

int CDVDAudioCodecPassthrough::GetBitsPerSample()
{
  return OUT_SAMPLESIZE;
}

