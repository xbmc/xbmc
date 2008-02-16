#include "stdafx.h"
#include "SPCCodec.h"
#include "../DllLoader/DllLoader.h"
#include "../../MusicInfoTagLoaderSPC.h"

using namespace MUSIC_INFO;
using namespace XFILE;

SPCCodec::SPCCodec()
{
  m_CodecName = "SPC";
  m_szBuffer = NULL;
  m_pApuRAM = NULL;
  m_iDataPos = 0; 
  m_loader = NULL;
  m_dll.EmuAPU = NULL;
  m_dll.LoadSPCFile = NULL;
  m_dll.SeekAPU = NULL;
}

SPCCodec::~SPCCodec()
{
  DeInit();
}

bool SPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_loader = new DllLoader("Q:\\system\\players\\paplayer\\snesapu.dll");
  if (!m_loader)
    return false;
  if (!m_loader->Load())
  {
    delete m_loader;
    m_loader = NULL;
    return false;
  }
  
  m_loader->ResolveExport("LoadSPCFile",(void**)&m_dll.LoadSPCFile);
  m_loader->ResolveExport("EmuAPU",(void**)&m_dll.EmuAPU);
  m_loader->ResolveExport("SeekAPU",(void**)&m_dll.SeekAPU);
  
  CFile file;
  if (!file.Open(strFile))
  {
    CLog::Log(LOGERROR,"SPCCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_szBuffer = new char[0x10200];
  if (!file.Read(m_szBuffer,0x10200))
  {
    delete[] m_szBuffer;
    m_szBuffer = NULL;
    file.Close();
    CLog::Log(LOGERROR,"SPCCodec: error reading file %s!",strFile.c_str());
    return false;
  }
  file.Close();

  m_pApuRAM = new u8[65536];

  m_dll.LoadSPCFile(m_szBuffer);
 
  m_SampleRate = 32000;  
  m_Channels = 2;
  m_BitsPerSample = 16;
  CMusicInfoTagLoaderSPC tagLoader;
  CMusicInfoTag tag;
  tagLoader.Load(strFile,tag);
  if (tag.Loaded())
    m_TotalTime = tag.GetDuration()*1000;
  else
    m_TotalTime = 4*60*1000; // default
  m_iDataPos = 0;
 
  return true;
}

void SPCCodec::DeInit()
{ 
  if (m_loader)
    delete m_loader;
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
  
  if (m_pApuRAM)
    delete[] m_pApuRAM;
  m_pApuRAM = NULL;
}

__int64 SPCCodec::Seek(__int64 iSeekTime)
{
  if (m_iDataPos > iSeekTime/1000*m_SampleRate*4)
  {
    m_dll.LoadSPCFile(m_szBuffer);
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
  }
  else
  {
    __int64 iDataPos2 = m_iDataPos;
    m_iDataPos = iSeekTime/1000*m_SampleRate*4;
    iSeekTime -= (iDataPos2*1000)/(m_SampleRate*4);
  }
  
  m_dll.SeekAPU((u32)iSeekTime*64,0);
  return (m_iDataPos*1000)/(m_SampleRate*4);
}

int SPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos >= m_TotalTime/1000*m_SampleRate*4)
    return READ_EOF;
  
  *actualsize = (int)((BYTE*)m_dll.EmuAPU(pBuffer,0,size/4)-pBuffer);
  m_iDataPos += *actualsize;

  if (*actualsize)
    return READ_SUCCESS;    
  else
    return READ_ERROR;
}

bool SPCCodec::CanInit()
{
  return true;
}

