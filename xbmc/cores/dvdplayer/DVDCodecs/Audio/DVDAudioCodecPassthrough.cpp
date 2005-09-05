
#include "../../../../stdafx.h"
#include ".\dvdaudiocodecpassthrough.h"
#include "../../../../XBAudioConfig.h"

#define EMULATE_INTTYPES
#include "..\..\ffmpeg\avcodec.h"


#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#ifdef __cplusplus
extern "C"
{
#endif
#  include "DTSHelper.h"
#ifdef __cplusplus
}
#endif


CDVDAudioCodecPassthrough::CDVDAudioCodecPassthrough(void)
{
  m_pPassBuffer = NULL;
  m_pLiba52 = NULL;
  m_iPassBufferLen = 0;

  m_pDataFrame = NULL;
  m_iDataFrameAlloced = 0;
  m_iDataFrameLen = 0;
  m_iChunkSize=0;
  m_iOffset=0;

  m_iFrameSize=0;
  m_iType = ENS_UNKNOWN;

  m_pPassBuffer = mPassBuffer;
  m_pDataFrame = (BYTE*)malloc(6144);
  m_iDataFrameAlloced = 6144;
  if ( !m_pPassBuffer ) CLog::Log(LOGERROR, "CDVDAudioCodecPassthrough: Unable to allocate buffer, crash imminent");

}

CDVDAudioCodecPassthrough::~CDVDAudioCodecPassthrough(void)
{
  Dispose();
}


bool CDVDAudioCodecPassthrough::SyncDTSHeader(BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize )
{
    int i = 0, iLen = 0;
    int iSampleRate, m_BitRate;

    while (i <= (iDataSize - 10))
    {
      iLen = helper_dts_syncinfo(pData, NULL, &iSampleRate, &m_BitRate);
      if (iLen > 0)
      {
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
    //Setup ac3 header
    pOut[0] = 0x72;
    pOut[1] = 0xF8;
    pOut[2] = 0x1F;
    pOut[3] = 0x4E;
    pOut[4] = 0x01; //(length) ? data_type : 0; /* & 0x1F; */
    pOut[5] = 0x00;
    pOut[6] = (iDataSize << 3) & 0xFF;
    pOut[7] = (iDataSize >> 5) & 0xFF;

    //Swap byteorder
    swab((char*)pData, (char*)pOut+8, iDataSize);
    memset(pOut + iDataSize + 8, 0, 6144 - iDataSize - 8);
    return 6144;
}

bool CDVDAudioCodecPassthrough::Open(CodecID codecID, int iChannels, int iSampleRate, int iBits)
{
  bool bSupportsAC3Out = false,  bSupportsDTSOut = false;

  if (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL)
  {
    bSupportsAC3Out = g_audioConfig.GetAC3Enabled();
    bSupportsDTSOut = g_audioConfig.GetDTSEnabled();
  }

  //Samplerate cannot be checked here as we don't know it at this point in time. 
  //We should probably have a way to try to decode data so that we know what samplerate it is.

  if ((codecID == CODEC_ID_AC3 && bSupportsAC3Out) || (codecID == CODEC_ID_DTS && bSupportsDTSOut))
  {
    m_pLiba52 = new CDVDAudioCodecLiba52();
    m_pLiba52->Open(codecID, iChannels, iSampleRate, iBits);
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
  if( m_pLiba52 )
  {
    m_pLiba52->Dispose();
    delete m_pLiba52;
    m_pLiba52 = NULL;
  }
}

int CDVDAudioCodecPassthrough::Decode(BYTE* pData, int iSize)
{
  //Still have data to portion out.
  if( m_iPassBufferLen > 0) return 0;
  m_pPassBuffer = mPassBuffer;
  
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
      if(m_iFrameSize >= 10 && m_iFrameSize <= 6144-8) 
      {
        if(m_iType != ENS_DTS)
        {
          CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough: Synced to DTS frame");
          m_iType = ENS_DTS;
        }
      }
      else
      {
        m_iFrameSize = 0;
        m_iDataFrameLen = 0; //Discard data
        CLog::Log(LOGWARNING, "CDVDAudioCodecPassthrough::Decode - Error: Invalid DTS framesize");
        return -1;
      }
    }

    if( m_pLiba52->SyncAC3Header(m_pDataFrame, m_iDataFrameLen, &m_iOffset, &m_iFrameSize) )
    {
      if(m_iFrameSize >= 7 && m_iFrameSize <= 3840) 
      {
        if(m_iType != ENS_AC3)
        {
          CLog::Log(LOGDEBUG, "CDVDAudioCodecPassthrough: Synced to AC3 frame");
          m_iType = ENS_AC3;
        }
        if (m_pLiba52->GetSampleRate() != 48000)
        {
          m_iFrameSize = 0;
          m_iDataFrameLen = 0; //Discard data
          CLog::Log(LOGWARNING, "CDVDAudioCodecPassthrough::Decode - Error: Invalid AC3 samplerate, passtrough only supports 480000khz");
          return -1;
        }
      }
      else
      {
        m_iFrameSize = 0;
        m_iDataFrameLen = 0; //Discard data
        CLog::Log(LOGWARNING, "CDVDAudioCodecPassthrough::Decode - Error: Invalid AC3 framesize");
        return -1;
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
    if(m_iType == ENS_DTS)
    {
      m_iPassBufferLen = decode_audio_dts(m_pDataFrame + m_iOffset, m_iFrameSize, m_pPassBuffer);
      m_iChunkSize = m_iPassBufferLen;
    }
    else if(m_iType == ENS_AC3)
    {
      m_iPassBufferLen = PaddAC3Data(m_pDataFrame + m_iOffset, m_iFrameSize, m_pPassBuffer);
      m_iChunkSize = m_iPassBufferLen;
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
    if(m_iPassBufferLen < m_iChunkSize)
      m_iChunkSize = m_iPassBufferLen;

    m_iPassBufferLen-= m_iChunkSize;
    m_pPassBuffer+=m_iChunkSize;
    return m_iChunkSize;
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
  return 2;
}

int CDVDAudioCodecPassthrough::GetSampleRate()
{
  return 48000;
}

int CDVDAudioCodecPassthrough::GetBitsPerSample()
{
  return 16;
}

