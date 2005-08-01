
#include "../../stdafx.h"
#include "DllLoaderContainer.h"

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

HMODULE DllLoaderContainer::GetModuleAddress(char* sName)
{
  return (HMODULE)GetModule(sName);
}

DllLoader* DllLoaderContainer::GetModule(char* sName)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (stricmp(m_dlls[i]->GetName(), sName) == 0) return m_dlls[i];
    if (!m_dlls[i]->IsSystemDll() && stricmp(m_dlls[i]->GetFileName(), sName) == 0) return m_dlls[i];
  }
  return NULL;
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
