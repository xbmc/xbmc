#include "stdafx.h"
#ifdef HAS_WMA_CODEC
#include "WMACodec.h"
#include "../../Util.h"

WMACodec::WMACodec()
{
  m_CodecName = "WMA";
  m_iDataPos = -1; 
  m_hnd = NULL;
}

WMACodec::~WMACodec()
{
  DeInit();
}

bool WMACodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_dll.Init();

  m_hnd = m_dll.LoadFile(strFile.c_str(), &m_TotalTime, &m_SampleRate, &m_BitsPerSample, &m_Channels);
  if (m_hnd == 0)
    return false;
 
  // We always ask ffmpeg to return s16le 
  m_BitsPerSample = 16;
 
  return true;
}

void WMACodec::DeInit()
{
  if (m_hnd != NULL)
  {
    m_dll.UnloadFile(m_hnd);
    m_hnd = NULL;
  }
}

__int64 WMACodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_hnd, (unsigned long)iSeekTime);
  m_iDataPos = result/1000*m_SampleRate*m_BitsPerSample*m_Channels/8;
  
  return result;
}

int WMACodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*m_BitsPerSample*m_Channels/8)
  {
    return READ_EOF;
  }

  if ((*actualsize=m_dll.FillBuffer(m_hnd, (char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool WMACodec::CanInit()
{
  return m_dll.CanLoad();
}

#endif

