#include "stdafx.h"
#include "DVDPlayerCodec.h"
#include "../../Util.h"

#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDFactoryCodec.h"

DVDPlayerCodec::DVDPlayerCodec()
{
  m_CodecName = "DVDPlayer";
  m_iDataPos = -1; 
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
  m_pAudioCodec = NULL;
  m_audioLen = 0;
  m_audioPos = 0;
  m_audioData = NULL;
  }

DVDPlayerCodec::~DVDPlayerCodec()
{
  DeInit();
}

void DVDPlayerCodec::SetContentType(const CStdString &strContent)
{
  m_strContentType = strContent;
}

bool DVDPlayerCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  CStdString strFileToOpen = strFile;

  CURL urlFile(strFile);
  if (urlFile.GetProtocol() == "shout" )
    strFileToOpen.Replace("shout://","http://");

  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strFileToOpen, m_strContentType);
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating input stream for %s", __FUNCTION__, strFileToOpen.c_str());
    return false;
  }

  if (!m_pInputStream->Open(strFileToOpen.c_str(), m_strContentType))
  {
    CLog::Log(LOGERROR, "%s: Error opening file %s", __FUNCTION__, strFileToOpen.c_str());
    if (m_pInputStream)
      delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  m_pDemuxer = NULL;
  
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if (!m_pDemuxer)
    {
      delete m_pInputStream;
      m_pInputStream = NULL;
      CLog::Log(LOGERROR, "%s: Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s: Exception thrown when opeing demuxer", __FUNCTION__);
    if (m_pDemuxer)
    {
      delete m_pDemuxer;
      m_pDemuxer = NULL;
    }
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }    

  CDemuxStream* pStream = NULL;
  m_nAudioStream = -1;
  for (int i = 0; i < m_pDemuxer->GetNrOfStreams(); i++)
  {
    pStream = m_pDemuxer->GetStream(i);
    if (pStream && pStream->type == STREAM_AUDIO)
    {
      m_nAudioStream = i;
      break;
    }
  }

  if (m_nAudioStream == -1)
  {
    CLog::Log(LOGERROR, "%s: Could not find audio stream", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }
 
  CDVDStreamInfo hint(*pStream, true);
  m_pAudioCodec = CDVDFactoryCodec::CreateAudioCodec(hint);
  if (!m_pAudioCodec)
  {
    CLog::Log(LOGERROR, "%s: Could not create audio codec", __FUNCTION__);
    delete m_pDemuxer;
    m_pDemuxer = NULL;
    delete m_pInputStream;
    m_pInputStream = NULL;
    return false;
  }

  // we have to decode initial data in order to get channels/samplerate
  // for sanity - we read no more than 10 packets
  DemuxPacket* pPacket;
  for (int nPacket=0; nPacket < 10 && (m_Channels == 0 || m_SampleRate == 0); nPacket++)
  {
    do
    {
      pPacket = m_pDemuxer->Read();
    } while (pPacket && pPacket->iStreamId != m_nAudioStream);

    // dummy data - just to get the channels/sample rate
    if (pPacket)
    {
      m_pAudioCodec->Decode(pPacket->pData, pPacket->iSize);
      m_pAudioCodec->GetData(&m_audioData);

      // We always ask ffmpeg to return s16le 
      m_BitsPerSample = m_pAudioCodec->GetBitsPerSample();
      m_SampleRate = m_pAudioCodec->GetSampleRate();
      m_Channels = m_pAudioCodec->GetChannels();

      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
    }
    else 
      break;
  }

  if (m_Channels == 0) // no data - just guess and hope for the best
    m_Channels = 2;

  if (m_SampleRate == 0)
    m_SampleRate = 44100;

  return true;
}

void DVDPlayerCodec::DeInit()
{
  if (m_pDemuxer != NULL)
  {
    delete m_pDemuxer;
    m_pDemuxer = NULL;
  }
    
  if (m_pInputStream != NULL)
  {
    delete m_pInputStream;
    m_pInputStream = NULL;
  }
  
  if (m_pAudioCodec != NULL)
  {
    delete m_pAudioCodec;
    m_pAudioCodec = NULL;
  }
  
  m_iDataPos = -1;
  m_audioLen = 0;
  m_audioPos = 0;
  m_audioData = NULL;
}

__int64 DVDPlayerCodec::Seek(__int64 iSeekTime)
{  
  m_pDemuxer->SeekTime(iSeekTime, true);
  m_audioLen = 0;
  m_audioPos = 0;
  m_audioData = NULL;
  return iSeekTime;
}

int DVDPlayerCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }
  
  if (m_audioLen > 0 && m_audioPos < m_audioLen)
  {
    *actualsize = m_audioLen - m_audioPos;
    if (*actualsize > size)
      *actualsize = size;
    memcpy(pBuffer,       m_audioData + m_audioPos, *actualsize);
    m_audioPos += *actualsize;
    return READ_SUCCESS;
  }

  DemuxPacket* pPacket = NULL;

  do
  {
    pPacket = m_pDemuxer->Read();
  } while (pPacket && pPacket->iStreamId != m_nAudioStream);

  if (!pPacket)
  {
    return READ_EOF;
  }
  
  int decodeLen = m_pAudioCodec->Decode(pPacket->pData, pPacket->iSize);
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);

  if (decodeLen < 0)
    return READ_ERROR;
  
  m_audioLen = m_pAudioCodec->GetData(&m_audioData);
  *actualsize = (m_audioLen > size ? size : m_audioLen);
  m_audioPos = *actualsize;
  memcpy(pBuffer, m_audioData, *actualsize);
  
  m_iDataPos += *actualsize;
  
  return READ_SUCCESS;
}

bool DVDPlayerCodec::CanInit()
{
  return true;
}

bool DVDPlayerCodec::CanSeek()
{
  return true;
}
