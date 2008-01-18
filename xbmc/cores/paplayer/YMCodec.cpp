#include "stdafx.h"
#include "YMCodec.h"
#include "../DllLoader/DllLoader.h"
#include "../../Util.h"

YMCodec::YMCodec()
{
  m_CodecName = "YM";
  m_ym = 0;
  m_iDataPos = -1; 
}

YMCodec::~YMCodec()
{
  DeInit();
}

bool YMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
   
  m_ym = m_dll.LoadYM(strFile.c_str());
  if (!m_ym)
  {
    CLog::Log(LOGERROR,"YMCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_Channels = 1;
  m_SampleRate = 44100;
  m_BitsPerSample = 16;
  m_TotalTime = m_dll.GetLength(m_ym)*1000;

  return true;
}

void YMCodec::DeInit()
{
  if (m_ym)
    m_dll.FreeYM(m_ym);
  m_ym = 0;
}

__int64 YMCodec::Seek(__int64 iSeekTime)
{
  return m_dll.Seek(m_ym,(unsigned long)iSeekTime);
}

int YMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if ((*actualsize=m_dll.FillBuffer(m_ym,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool YMCodec::CanInit()
{
  return m_dll.CanLoad();
}

