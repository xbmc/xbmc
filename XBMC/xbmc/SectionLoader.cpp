#include "stdafx.h"
#include "sectionloader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CSectionLoader g_sectionLoader;

//  delay for unloading dll's
#define UNLOAD_DELAY 30*1000 // 30 sec.

CSectionLoader::CSectionLoader(void)
{}

CSectionLoader::~CSectionLoader(void)
{}

bool CSectionLoader::IsLoaded(const CStdString& strSection)
{
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedSections.size(); ++i)
  {
    CSection& section = g_sectionLoader.m_vecLoadedSections[i];
    if (section.m_strSectionName == strSection) return true;
  }
  return false;
}

bool CSectionLoader::Load(const CStdString& strSection)
{
  CStdString strLog;
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedSections.size(); ++i)
  {
    CSection& section = g_sectionLoader.m_vecLoadedSections[i];
    if (section.m_strSectionName == strSection)
    {
      section.m_lReferenceCount++;
      strLog.Format("SECTION:LoadSection(%s) count:%i\n", strSection.c_str(), section.m_lReferenceCount);
      OutputDebugString(strLog);
      return true;
    }
  }

  if ( NULL == XLoadSection(strSection.c_str() ) )
  {
    strLog.Format("SECTION:LoadSection(%s) load failed!!\n", strSection.c_str());
    OutputDebugString(strLog.c_str() );
    return false;
  }
  HANDLE hHandle = XGetSectionHandle(strSection.c_str());

  strLog.Format("SECTION:Section %s loaded count:1 size:%i\n", strSection.c_str(), XGetSectionSize(hHandle) );
  OutputDebugString(strLog.c_str() );

  CSection newSection;
  newSection.m_strSectionName = strSection;
  newSection.m_lReferenceCount = 1;
  g_sectionLoader.m_vecLoadedSections.push_back(newSection);
  return true;
}

void CSectionLoader::Unload(const CStdString& strSection)
{
  if (!CSectionLoader::IsLoaded(strSection)) return ;

  ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section = *i;
    if (section.m_strSectionName == strSection)
    {
      CStdString strLog;
      strLog.Format("SECTION:FreeSection(%s) count:%i\n", strSection.c_str(), section.m_lReferenceCount);
      OutputDebugString(strLog);
      section.m_lReferenceCount--;
      if ( 0 == section.m_lReferenceCount)
      {

        strLog.Format("SECTION:FreeSection(%s) deleted\n", strSection.c_str());
        OutputDebugString(strLog);
        g_sectionLoader.m_vecLoadedSections.erase(i);
        XFreeSection(strSection.c_str());

        return ;
      }
    }
    ++i;
  }
}

DllLoader *CSectionLoader::LoadDLL(const CStdString &dllname)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (!dllname) return NULL;
  // check if it's already loaded, and increase the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_strDllName == dllname)
    {
      dll.m_lReferenceCount++;
      return dll.m_pDll;
    }
  }
  // ok, now load the dll and resolve imports as necessary
  CDll newDLL;
  newDLL.m_strDllName = dllname;
  newDLL.m_lReferenceCount = 1;
  newDLL.m_pDll = new DllLoader(dllname, true);
  if (newDLL.m_pDll && newDLL.m_pDll->Parse())
  {
    newDLL.m_pDll->ResolveImports();
    g_sectionLoader.m_vecLoadedDLLs.push_back(newDLL);
    CStdString strLog;
    strLog.Format("SECTION:LoadDLL(%s)\n", dllname.c_str());
    OutputDebugString(strLog);
    return newDLL.m_pDll;
  }
  return NULL;
}

void CSectionLoader::UnloadDLL(const CStdString &dllname)
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  if (!dllname) return;
  // check if it's already loaded, and increase the reference count if so
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_strDllName == dllname)
    {
      dll.m_lReferenceCount--;
      if (dll.m_lReferenceCount == 0)
      {
        dll.m_lUnloadDelayStartTick=GetTickCount();
        return;
      }
    }
  }
}

void CSectionLoader::UnloadDLLsDelayed()
{
  CSingleLock lock(g_sectionLoader.m_critSection);

  // check if we can unload any unreferenced dlls
  for (int i = 0; i < (int)g_sectionLoader.m_vecLoadedDLLs.size(); ++i)
  {
    CDll& dll = g_sectionLoader.m_vecLoadedDLLs[i];
    if (dll.m_lReferenceCount == 0 && GetTickCount()-dll.m_lUnloadDelayStartTick>UNLOAD_DELAY)
    {
      CStdString strLog;
      strLog.Format("SECTION:UnloadDLLsDelayed(%s)\n", dll.m_strDllName.c_str());
      OutputDebugString(strLog);
      if (dll.m_pDll) delete dll.m_pDll;
      g_sectionLoader.m_vecLoadedDLLs.erase(g_sectionLoader.m_vecLoadedDLLs.begin() + i);
      return;
    }
  }
}

void CSectionLoader::UnloadAll()
{
  ivecLoadedSections i;
  i = g_sectionLoader.m_vecLoadedSections.begin();
  while (i != g_sectionLoader.m_vecLoadedSections.end())
  {
    CSection& section = *i;
    //g_sectionLoader.m_vecLoadedSections.erase(i);
    OutputDebugString("SECTION:UnloadAll:");
    OutputDebugString(section.m_strSectionName.c_str());
    OutputDebugString("\n");
    XFreeSection(section.m_strSectionName.c_str());
    i = g_sectionLoader.m_vecLoadedSections.erase(i);
  }

  // delete the dll's
  CSingleLock lock(g_sectionLoader.m_critSection);
  vector<CDll>::iterator it = g_sectionLoader.m_vecLoadedDLLs.begin();
  while (it != g_sectionLoader.m_vecLoadedDLLs.end())
  {
    CDll& dll = *it;
    if (dll.m_pDll)
      delete dll.m_pDll;
    it = g_sectionLoader.m_vecLoadedDLLs.erase(it);
  }
}
