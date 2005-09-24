#include "../../stdafx.h"
#include "SPCCodec.h"
#include "../DllLoader/DllLoader.h"

SPCCodec::SPCCodec()
{
  m_CodecName = L"SPC";
  m_iDataInBuffer = 0;
  m_szBuffer = NULL;
  m_spc = 0;
  m_iDataPos = 0; 
}

SPCCodec::~SPCCodec()
{
  DeInit();
}

bool SPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_iBufferSize = m_dll.Init();
  
  m_spc = m_dll.LoadSPC(strFile.c_str());
  if (!m_spc)
  {
    CLog::Log(LOGERROR,"SPCCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_strFile = strFile;
  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = m_dll.GetLength(m_spc)*1000;
  m_szBuffer = new char[m_iBufferSize];
  m_iDataPos = 0;

  return true;
}

void SPCCodec::DeInit()
{
  if (m_spc)
    m_dll.FreeSPC(m_spc);
  m_spc = 0;
  
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
}

__int64 SPCCodec::Seek(__int64 iSeekTime)
{
  if (m_iDataPos > iSeekTime/1000*48000*4)
  {
    m_dll.FreeSPC(m_spc);
    m_spc = m_dll.LoadSPC(m_strFile.c_str());
    m_iDataPos = 0;
    m_szStartOfBuffer = m_szBuffer;
  }
  while (m_iDataPos+2*m_iBufferSize < iSeekTime/1000*48000*4)
  {
    m_dll.FrameAdvance(m_spc);
    
    m_iDataInBuffer = m_iBufferSize;
    m_szStartOfBuffer = m_szBuffer;
    m_iDataPos += m_iBufferSize;
  }
  m_dll.FillBuffer(m_spc,m_szBuffer);
  
  if (iSeekTime/1000*48000*4 > m_iBufferSize)    
    m_iDataPos += m_iBufferSize;
  else
    m_iDataPos = 0;

  m_iDataInBuffer -= int(iSeekTime/1000*48000*4-m_iDataPos);
  m_szStartOfBuffer += (iSeekTime/1000*48000*4-m_iDataPos);
  m_iDataPos = iSeekTime/1000*48000*4;

  return iSeekTime;
}

int SPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataInBuffer <= 0)
  {
    m_dll.FillBuffer(m_spc,m_szBuffer);
    m_iDataInBuffer = m_iBufferSize;
    m_szStartOfBuffer = m_szBuffer;
  }

  *actualsize= size<m_iDataInBuffer?size:m_iDataInBuffer;
  memcpy(pBuffer,m_szStartOfBuffer,*actualsize);
  m_szStartOfBuffer += *actualsize;
  m_iDataInBuffer -= *actualsize;
  m_iDataPos += *actualsize;

  return READ_SUCCESS;    
}

bool SPCCodec::CanInit()
{
  return m_dll.CanLoad();
}