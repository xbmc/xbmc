#include "stdafx.h"
#include "../../Util.h"

#include "ADPCMCodec.h"

ADPCMCodec::ADPCMCodec()
{
  m_CodecName = "ADPCM";
  m_adpcm = 0;
  m_bIsPlaying = false;
} 

ADPCMCodec::~ADPCMCodec()
{
  DeInit();
}

bool ADPCMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  DeInit();

  if (!m_dll.Load())
    return false; // error logged previously
    
  m_adpcm = m_dll.LoadXWAV(strFile.c_str());
  if (!m_adpcm)
  {
    CLog::Log(LOGERROR,"ADPCMCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_Channels = m_dll.GetNumberOfChannels(m_adpcm);
  m_SampleRate = m_dll.GetPlaybackRate(m_adpcm);
  m_BitsPerSample = 16;//m_dll.GetSampleSize(m_adpcm);
  m_TotalTime = m_dll.GetLength(m_adpcm); // fixme?
  m_iDataPos = 0;

  return true;
}

void ADPCMCodec::DeInit()
{
  if (m_adpcm)
    m_dll.FreeXWAV(m_adpcm);

  m_adpcm = 0;
  m_bIsPlaying = false;
} 

__int64 ADPCMCodec::Seek(__int64 iSeekTime)
{
  m_dll.Seek(m_adpcm,(int)iSeekTime);
  return iSeekTime;
}

int ADPCMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_adpcm)
    return READ_ERROR;
  
  *actualsize  = m_dll.FillBuffer(m_adpcm,(char*)pBuffer,size);

  if (*actualsize == 0)
    return READ_ERROR;

  return READ_SUCCESS;    
}

bool ADPCMCodec::CanInit()
{
  return m_dll.CanLoad();
}

