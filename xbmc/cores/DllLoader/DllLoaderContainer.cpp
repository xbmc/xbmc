
#include "../../stdafx.h"
#include "DllLoaderContainer.h"

#define ENV_PATH "Q:\\system\\;Q:\\system\\players\\mplayer\\;Q:\\system\\players\\dvdplayer\\;Q:\\system\\players\\paplayer\\"

#define DLL_PROCESS_DETACH   0
#define DLL_PROCESS_ATTACH   1
#define DLL_THREAD_ATTACH    2
#define DLL_THREAD_DETACH    3
#define DLL_PROCESS_VERIFIER 4

//  Entry point of a dll (DllMain)
typedef BOOL WINAPI EntryFunc(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

//Define this to get loggin on all calls to load/unload of dlls
//#define LOGALL

DllLoaderContainer g_dlls;

void export_reg();
void export_ole32();
void export_xbp();
void export_winmm();
void export_user32();
void export_msdmo();
void export_xbmc_vobsub();
void export_kernel32();
void export_wsock32();
void export_ws2_32();
void export_xbox_dx8();
void export_xbox___dx8();
void export_version();
void export_comdlg32();
void export_gdi32();
void export_ddraw();
void export_comctl32();
void export_msvcrt();
void export_msvcr71();
void export_pncrt();
void export_python23();

DllLoaderContainer::DllLoaderContainer() :
    kernel32("kernel32.dll", false, true),
    msvcr71("msvcr71.dll", false, true),
    msvcrt("msvcrt.dll", false, true),
    wsock32("wsock32.dll", false, true),
    ws2_32("ws2_32.dll", false, true),
    user32("user32.dll", false, true),
    ddraw("ddraw.dll", false, true),
    wininet("wininet.dll", false, true),
    advapi32("advapi32.dll", false, true),
    ole32("ole32.dll", false, true),
    xbp("xbp.dll", false, true),
    winmm("winmm.dll", false, true),
    msdmo("msdmo.dll", false, true),
    xbmc_vobsub("xbmc_vobsub.dll", false, true),
    xbox_dx8("xbox_dx8.dll", false, true),
    xbox___dx8("xbox-dx8.dll", false, true),
    version("version.dll", false, true),
    comdlg32("comdlg32.dll", false, true),
    gdi32("gdi32.dll", false, true),
    comctl32("comctl32.dll", false, true),
    pncrt("pncrt.dll", false, true),
    python23("python23.dll", false, true)
{
  m_iNrOfDlls = 0;
  m_bTrack = true;
  
  RegisterDll(&kernel32); export_kernel32();
  RegisterDll(&msvcr71); export_msvcr71();
  RegisterDll(&msvcrt); export_msvcrt();
  RegisterDll(&wsock32); export_wsock32();
  RegisterDll(&ws2_32); export_ws2_32();
  RegisterDll(&user32); export_user32();
  RegisterDll(&ddraw); export_ddraw();
  RegisterDll(&wininet); // nothing is exported in this dll, is this one really needed?
  RegisterDll(&advapi32); export_reg();
  RegisterDll(&ole32); export_ole32();
  RegisterDll(&xbp); //export_xbp();
  RegisterDll(&winmm); export_winmm();
  RegisterDll(&msdmo); export_msdmo();
  RegisterDll(&xbmc_vobsub); export_xbmc_vobsub();
  RegisterDll(&xbox_dx8); export_xbox_dx8();
  RegisterDll(&xbox___dx8); export_xbox___dx8();
  RegisterDll(&version); export_version();
  RegisterDll(&comdlg32); export_comdlg32();
  RegisterDll(&gdi32); export_gdi32();
  RegisterDll(&comctl32); export_comctl32();
  RegisterDll(&pncrt); export_pncrt();
  RegisterDll(&python23); export_python23();
}
  
void DllLoaderContainer::Clear()
{
}

HMODULE DllLoaderContainer::GetModuleAddress(const char* sName)
{
  return (HMODULE)GetModule(sName);
}

DllLoader* DllLoaderContainer::GetModule(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }

  return NULL;
}

DllLoader* DllLoaderContainer::LoadModule(const char* sName, const char* sCurrentDir/*=NULL*/)
{
  DllLoader* pDll=NULL;

  if (IsSystemDll(sName))
  {
    pDll = g_dlls.GetModule(sName);
  }
  else if (sCurrentDir)
  {
    CStdString strPath=sCurrentDir;
    strPath+=sName;
    pDll = g_dlls.GetModule(strPath.c_str());
  }
  else
  {
    pDll = g_dlls.GetModule(sName);
  }

  if (!pDll)
  {
    pDll=g_dlls.FindModule(sName, sCurrentDir);
  }
  else if (!pDll->IsSystemDll())
  {
    pDll->IncrRef();

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Already loaded Dll %s at 0x%x", pDll->GetFileName(), pDll);
#endif

  }

  return pDll;
}

DllLoader* DllLoaderContainer::FindModule(const char* sName, const char* sCurrentDir)
{
  if (strlen(sName) > 1 && sName[1] == ':')
  { //  Has a path, just try to load
    return LoadDll(sName);
  }
  else if (sCurrentDir)
  { // in the path of the parent dll?
    CStdString strPath=sCurrentDir;
    strPath+=sName;

    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str());
  }

  //  in environment variable?
  CStdStringArray vecEnv;
  StringUtils::SplitString(ENV_PATH, ";", vecEnv);

  for (int i=0; i<(int)vecEnv.size(); ++i)
  {
    CStdString strPath=vecEnv[i];

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Searching for the dll %s in directory %s", sName, strPath.c_str());
#endif

    strPath+=sName;
    if (CFile::Exists(strPath))
      return LoadDll(strPath.c_str());
  }

  CLog::Log(LOGERROR, "Dll %s was not found in path", sName);

  return NULL;
}

void DllLoaderContainer::ReleaseModule(DllLoader*& pDll)
{
  if (pDll->IsSystemDll())
  {
    CLog::Log(LOGFATAL, "%s is a system dll and should never be released", pDll->GetName());
    return;
  }

  int iRefCount=pDll->DecrRef();
  if (iRefCount==0)
  {

#ifdef LOGALL
    CLog::Log(LOGDEBUG, "Releasing Dll %s", pDll->GetFileName());
    CLog::Log(LOGDEBUG, "Executing EntryPoint with DLL_PROCESS_DETACH at: 0x%x - Dll: %s", pDll->EntryAddress, pDll->GetFileName());
#endif

    //call "DllMain" with DLL_PROCESS_DETACH
    EntryFunc* initdll = (EntryFunc *)pDll->EntryAddress;
    (*initdll)((HINSTANCE)pDll->hModule, DLL_PROCESS_DETACH , 0);

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "EntryPoint with DLL_PROCESS_DETACH called - Dll: %s", pDll->GetFileName());
#endif

    delete pDll;
    pDll=NULL;
  }
#ifdef LOGALL
  else
  {
    CLog::Log(LOGDEBUG, "Dll %s is still referenced with a count of %d", pDll->GetFileName(), iRefCount);
  }
#endif
}

DllLoader* DllLoaderContainer::LoadDll(const char* sName)
{

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Loading dll %s", sName);
#endif

  DllLoader* pLoader=new DllLoader(sName, m_bTrack);
  if (!pLoader)
  {
    CLog::Log(LOGERROR, "Unable to create dll %s", sName);
    return NULL;
  }

  if (!pLoader->Parse())
  {
    CLog::Log(LOGERROR, "Unable to open dll %s", sName);
    delete pLoader;
    return NULL;
  }

  pLoader->ResolveImports();

  // only execute DllMain if no EntryPoint is found
  if (!pLoader->EntryAddress)
  {
    void* address = NULL;
    pLoader->ResolveExport("DllMain", &address);
    if (address) pLoader->EntryAddress = (unsigned long)address;
  }

  // patch some unwanted calls in memory
  if (strstr(sName, "QuickTime.qts"))
  {
    int i;
    DWORD dispatch_addr;
    DWORD imagebase_addr;
    DWORD dispatch_rva;

    pLoader->ResolveExport("theQuickTimeDispatcher", (void **)&dispatch_addr);
    imagebase_addr = (DWORD)pLoader->hModule;
    CLog::Log(LOGDEBUG, "Virtual Address of theQuickTimeDispatcher = 0x%x", dispatch_addr);
    CLog::Log(LOGDEBUG, "ImageBase of %s = 0x%x", sName, imagebase_addr);

    dispatch_rva = dispatch_addr - imagebase_addr;

    CLog::Log(LOGDEBUG, "Relative Virtual Address of theQuickTimeDispatcher = %p", dispatch_rva);

    DWORD base = imagebase_addr;
    if (dispatch_rva == 0x124C30)
    {
      CLog::Log(LOGINFO, "QuickTime5 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e842)[i] = 0x90; // make_new_region ?
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x19e86d)[i] = 0x90; // call__call_CreateCompatibleDC ?
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e898)[i] = 0x90; // jmp_to_call_loadbitmap ?
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x19e8ac)[i] = 0x90; // call__calls_OLE_shit ?
      for (i = 0;i < 106;i++) ((BYTE*)base + 0x261B10)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13B330)
    {
      CLog::Log(LOGINFO, "QuickTime6 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x2730CC)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x2730f7)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x273122)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x273131)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2AC852)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13C3E0)
    {
      CLog::Log(LOGINFO, "QuickTime6.3 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268F6C)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x268F97)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268FC2)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x268FD1)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2B4722)[i] = 0x90; // disable threads
    }
    else
    {
      CLog::Log(LOGERROR, "Unsupported QuickTime version");
    }

    CLog::Log(LOGINFO, "QuickTime.qts patched!!!\n");
  }

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "Executing EntryPoint with DLL_PROCESS_ATTACH at: 0x%x - Dll: %s", pLoader->EntryAddress, sName);
#endif

  EntryFunc* initdll = (EntryFunc *)pLoader->EntryAddress;
  (*initdll)((HINSTANCE) pLoader, DLL_PROCESS_ATTACH , 0); //call "DllMain" with DLL_PROCESS_ATTACH

#ifdef LOGALL
  CLog::Log(LOGDEBUG, "EntryPoint with DLL_PROCESS_ATTACH called - Dll: %s", sName);
#endif

  return pLoader;
}

bool DllLoaderContainer::IsSystemDll(const char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetName(), sName) == 0) return true;
  }

  return false;
}

int DllLoaderContainer::GetNrOfModules()
{
  return m_iNrOfDlls;
}

DllLoader* DllLoaderContainer::GetModule(int iPos)
{
  if (iPos < m_iNrOfDlls) return m_dlls[iPos];
  return NULL;
}

void DllLoaderContainer::RegisterDll(DllLoader* pDll)
{
  if (m_iNrOfDlls < sizeof(m_dlls))
  {
    m_dlls[m_iNrOfDlls] = pDll;
    m_iNrOfDlls++;
  }
}

void DllLoaderContainer::UnRegisterDll(DllLoader* pDll)
{
  if (pDll)
  {
    if (pDll->IsSystemDll())
    {
      CLog::Log(LOGFATAL, "%s is a system dll and should never be removed", pDll->GetName());
    }
    else
    {
      // remove from the list
      bool bRemoved = false;
      for (int i = 0; i < m_iNrOfDlls && m_dlls[i]; i++)
      {
        if (m_dlls[i] == pDll) bRemoved = true;
        if (bRemoved && i + 1 < m_iNrOfDlls)
        {
          m_dlls[i] = m_dlls[i + 1];
        }
      }
      if (bRemoved)
      {
        m_iNrOfDlls--;
        m_dlls[m_iNrOfDlls] = NULL;
      }
    }
  }
}
