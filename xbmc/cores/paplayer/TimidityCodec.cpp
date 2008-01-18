#include "stdafx.h"
#include "TimidityCodec.h"
#include "../DllLoader/DllLoader.h"
#include "../../Util.h"

TimidityCodec::TimidityCodec()
{
  m_CodecName = "MID";
  m_mid = 0;
  m_iTrack = -1;
  m_iDataPos = -1; 
  m_loader = NULL;
}

TimidityCodec::~TimidityCodec()
{
  DeInit();
}

bool TimidityCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_loader = new DllLoader("Q:\\system\\players\\paplayer\\timidity.dll");
  if (!m_loader)
    return false;
  if (!m_loader->Load())
  {
    delete m_loader;
    m_loader = NULL;
    return false;
  }
  
  m_loader->ResolveExport("DLL_Init",(void**)&m_dll.Init);
  m_loader->ResolveExport("DLL_LoadMID",(void**)&m_dll.LoadMID);
  m_loader->ResolveExport("DLL_FreeMID",(void**)&m_dll.FreeMID);
  m_loader->ResolveExport("DLL_FillBuffer",(void**)&m_dll.FillBuffer);
  m_loader->ResolveExport("DLL_GetLength",(void**)&m_dll.GetLength);
  m_loader->ResolveExport("DLL_Seek",(void**)&m_dll.Seek);
  m_dll.Init();
  
  CStdString strFileToLoad = "filereader://"+strFile;

  m_mid = m_dll.LoadMID(strFileToLoad.c_str());
  if (!m_mid)
  {
    CLog::Log(LOGERROR,"TimidityCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  
  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)m_dll.GetLength(m_mid);

  return true;
}

void TimidityCodec::DeInit()
{
  if (m_mid)
    m_dll.FreeMID(m_mid);
  if (m_loader)
    delete m_loader;
  m_mid = 0;
}

__int64 TimidityCodec::Seek(__int64 iSeekTime)
{
  __int64 result = (__int64)m_dll.Seek(m_mid,(unsigned long)iSeekTime);
  m_iDataPos = result/1000*48000*4;
  
  return result;
}

int TimidityCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos == -1)
  {
    m_iDataPos = 0;
  }

  if (m_iDataPos >= m_TotalTime/1000*48000*4)
    return READ_EOF;
  
  if ((*actualsize=m_dll.FillBuffer(m_mid,(char*)pBuffer,size))> 0)
  {
    m_iDataPos += *actualsize;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool TimidityCodec::CanInit()
{
  return XFILE::CFile::Exists("Q:\\system\\players\\paplayer\\timidity\\timidity.cfg");
}

bool TimidityCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "mid" || strExt == "kar")
    return true;
  
  return false;
}

