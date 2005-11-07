#include "../../stdafx.h"
#include "SPCCodec.h"
#include "../DllLoader/DllLoader.h"
#include "../../MusicInfoTagLoaderSPC.h"

HANDLE g_hSpcHandle = NULL;

SPCCodec::SPCCodec()
{
  m_CodecName = L"SPC";
  m_szBuffer = NULL;
  m_pDSP = NULL;
  m_pSPC = NULL;
  m_pApuRAM = NULL;
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

  if (!g_hSpcHandle)
    g_hSpcHandle = CreateMutex(NULL,false,NULL);
  WaitForSingleObject(g_hSpcHandle,INFINITE);

  m_pApuRAM = new u8[0x20000];

  m_dll.LoadSPCFile(m_szBuffer);
  
  m_dll.SetAPUOpt(1,2,16,48000,3,-1);
  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  CMusicInfoTagLoaderSPC tagLoader;
  CMusicInfoTag tag;
  tagLoader.Load(strFile,tag);
  if (tag.Loaded())
    m_TotalTime = tag.GetDuration()*1000;
  else
    m_TotalTime = 4*60*1000; // default
  m_iDataPos = 0;

  m_pDSP = new DSPState;
  m_pDSP->pDSP = new DSPReg;
  m_pDSP->pVoice = new Voice[8];
  m_pDSP->pEcho = new char[4*48000*2];

  m_pSPC = new SPCState;
  m_pSPC->pRAM = (u8*)(((u32)m_pApuRAM + 0xFFFF) & 0xFFFF0000);
  
  m_dll.SaveAPU(m_pSPC,m_pDSP);
  ReleaseMutex(g_hSpcHandle);
 
  return true;
}

void SPCCodec::DeInit()
{
  if (m_pDSP)
  {      
    delete m_pDSP->pDSP;
    delete[] m_pDSP->pVoice;
    delete[] m_pDSP->pEcho;
    delete m_pDSP;
  }
  m_pDSP = NULL;
  
  if (m_pSPC)
    delete m_pSPC;
  m_pSPC = NULL;
  
  if (m_szBuffer)
    delete[] m_szBuffer;
  m_szBuffer = NULL;
  
  if (m_pApuRAM)
    delete[] m_pApuRAM;
  m_pApuRAM = NULL;
}

__int64 SPCCodec::Seek(__int64 iSeekTime)
{
  WaitForSingleObject(g_hSpcHandle,INFINITE);
  m_dll.RestoreAPU(m_pSPC,m_pDSP);
  if (m_iDataPos > iSeekTime/1000*48000*4)
  {
    m_dll.LoadSPCFile(m_szBuffer);
    m_dll.SetAPUOpt(1,2,16,48000,3,-1);
    m_iDataPos = iSeekTime/1000*48000*4;
  }
  else
  {
    __int64 iDataPos2 = m_iDataPos;
    m_iDataPos = iSeekTime/1000*48000*4;
    iSeekTime -= (iDataPos2*1000)/(48000*4);
  }
  
  m_dll.SeekAPU((u32)iSeekTime*64,0);
  m_dll.SaveAPU(m_pSPC,m_pDSP);
  ReleaseMutex(g_hSpcHandle);
  return (m_iDataPos*1000)/(48000*4);
}

int SPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (m_iDataPos >= m_TotalTime/1000*48000*4)
    return READ_EOF;

  WaitForSingleObject(g_hSpcHandle,INFINITE);
  
  m_dll.RestoreAPU(m_pSPC,m_pDSP);
  *actualsize = (int)((BYTE*)m_dll.EmuAPU(pBuffer,size/4,1)-pBuffer);
  m_iDataPos += *actualsize;
  m_dll.SaveAPU(m_pSPC,m_pDSP);

  ReleaseMutex(g_hSpcHandle);
  if (*actualsize)
    return READ_SUCCESS;    
  else
    return READ_ERROR;
}

bool SPCCodec::CanInit()
{
  return m_dll.CanLoad();
}