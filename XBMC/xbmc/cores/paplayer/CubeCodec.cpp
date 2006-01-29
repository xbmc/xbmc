#include "../../stdafx.h"
#include "CubeCodec.h"
#include "../../Util.h"

CubeCodec::CubeCodec()
{
  m_CodecName = L"Cube";
  m_adx = 0;
  m_iDataPos = -1; 
}

CubeCodec::~CubeCodec()
{
  DeInit();
}

bool CubeCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false; // error logged previously
  
  m_dll.Init();

  CStdString strFileToLoad = strFile;
  
  m_adx = m_dll.LoadADX(strFileToLoad.c_str(),&m_SampleRate,&m_BitsPerSample,&m_Channels);
  if (!m_adx)
  {
    CLog::Log(LOGERROR,"CubeCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_TotalTime = (__int64)m_dll.GetLength(m_adx);

  return true;
}

void CubeCodec::DeInit()
{
  if (m_adx)
    m_dll.FreeADX(m_adx);
  m_adx = 0;
}

__int64 CubeCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_adx,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*m_SampleRate*m_BitsPerSample*m_Channels/8;
  
  return result;
}

int CubeCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*m_BitsPerSample*m_Channels/8)
  {
    return READ_EOF;
  }
  
  if ((*actualsize=m_dll.FillBuffer(m_adx,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool CubeCodec::CanInit()
{
  return m_dll.CanLoad();
}

bool CubeCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "adx" || strExt == "dsp" || strExt == "adp" || strExt == "ymf" || strExt == "ast" || strExt == "afc" || strExt == "hps")
    return true;
  
  return false;
}