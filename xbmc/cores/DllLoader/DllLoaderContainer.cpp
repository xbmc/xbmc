
#include "../../stdafx.h"
#include "DllLoaderContainer.h"

DllLoaderContainer g_dlls;

void DllLoaderContainer::Clear()
{
}

bool DllLoaderContainer::ContainsModule(HMODULE hModule)
{
  for (int i = 0; m_dlls[i] != NULL && i < m_iNrOfDlls; i++)
  {
    if (hModule == (HMODULE)m_dlls[i]) return true;
  }
  return false;
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
  if (pDll && pDll->IsSystemDll())
  {
    CLog::Log(LOGFATAL, "%s is a system dll and should never be removed", pDll->GetName());
  }
  
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
