#include "../../stdafx.h"
#include "../../util.h"
#include "ModuleCodec.h"

const CStdString strModuleDLL = "Q:\\system\\players\\paplayer\\dumb.dll";

bool ModuleCodec::IsSupportedFormat(const CStdString& strExt)
{
  if (strExt == "mod" || strExt == "it" || strExt == "s3m" || strExt == "duh" || strExt == "xm")
    return true;
  
  return false;
}

ModuleCodec::ModuleCodec()
{
  m_CodecName = L"MOD";
  m_module = 0;
  m_renderID = 0;
  m_bDllLoaded = false;
}

ModuleCodec::~ModuleCodec()
{
  DeInit();
}

bool ModuleCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  DeInit();

  if (!LoadDLL())
    return false; // error logged previously

  // set correct codec name
  CUtil::GetExtension(strFile,m_CodecName);
  m_CodecName.erase(0,1);
  m_CodecName.ToUpper();

  CStdString strLoadFile = "Z:\\cachedmod";
  if (!CUtil::IsHD(strFile))
    CFile::Cache(strFile,"Z:\\cachedmod");
  else
    strLoadFile = strFile;
  
  m_module = m_dll.DLL_LoadModule(strLoadFile.c_str());
  if (!m_module)
  {
    CLog::Log(LOGERROR,"ModuleCodec: error opening file %s!",strFile.c_str());
    return false;
  }
  m_Channels = 2;
  m_SampleRate = 48000;
  m_BitsPerSample = 16;
  m_TotalTime = (__int64)(m_dll.DLL_GetModuleLength(m_module))/65536*1000;

  return true;
}

void ModuleCodec::DeInit()
{
  if (m_bDllLoaded)
  {
    if (m_renderID)
      m_dll.DLL_StopPlayback(m_renderID);
    if (m_module)
      m_dll.DLL_FreeModule(m_module);

    CSectionLoader::UnloadDLL(strModuleDLL);
    m_renderID = 0;
    m_module = NULL;
    m_bDllLoaded = false;
  }
}

__int64 ModuleCodec::Seek(__int64 iSeekTime)
{
  if (m_renderID)
    m_dll.DLL_StopPlayback(m_renderID);
  
  m_renderID = m_dll.DLL_StartPlayback(m_module,long(iSeekTime/1000*65536));
  if (m_renderID)
    return iSeekTime;

  return -1;
}

int ModuleCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_module)
    return READ_ERROR;
  
  if (m_dll.DLL_GetModulePosition(m_renderID) >= m_dll.DLL_GetModuleLength(m_module))
    return READ_EOF;

  if (!m_renderID)
    m_renderID = m_dll.DLL_StartPlayback(m_module,0);

  if ((*actualsize=m_dll.DLL_FillBuffer(m_module,m_renderID,(char*)pBuffer,size,1.f)*4) == size)
    return READ_SUCCESS;
    
  return READ_ERROR;
}

bool ModuleCodec::CanInit()
{
  return CFile::Exists(strModuleDLL);
}

bool ModuleCodec::LoadDLL()
{
   if (m_bDllLoaded)
    return true;

  DllLoader* pDll = CSectionLoader::LoadDLL(strModuleDLL);
  if (!pDll)
  {
    CLog::Log(LOGERROR, "ModuleCodec: Unable to load dll %s", strModuleDLL.c_str());
    return false;
  }

  // get handle to the functions in the dll
  pDll->ResolveExport("DLL_LoadModule", (void**)&m_dll.DLL_LoadModule);
  pDll->ResolveExport("DLL_FreeModule", (void**)&m_dll.DLL_FreeModule);
  pDll->ResolveExport("DLL_GetModuleLength", (void**)&m_dll.DLL_GetModuleLength); 
   pDll->ResolveExport("DLL_GetModulePosition", (void**)&m_dll.DLL_GetModulePosition); 
 
  pDll->ResolveExport("DLL_StartPlayback", (void**)&m_dll.DLL_StartPlayback);
  pDll->ResolveExport("DLL_StopPlayback", (void**)&m_dll.DLL_StopPlayback);
  pDll->ResolveExport("DLL_FillBuffer", (void**)&m_dll.DLL_FillBuffer);

  // Check resolves + version number
  if ( !m_dll.DLL_FillBuffer || !m_dll.DLL_FreeModule || ! m_dll.DLL_LoadModule || !m_dll.DLL_StartPlayback || !m_dll.DLL_StopPlayback || !m_dll.DLL_GetModuleLength || !m_dll.DLL_GetModulePosition )
  {
    CLog::Log(LOGERROR, "ModuleCodec: Unable to resolve exports from %s", strModuleDLL);
    CSectionLoader::UnloadDLL(strModuleDLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}