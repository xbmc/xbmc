#pragma once

#include "cores/DllLoader/dll.h"

class CSectionLoader
{
public:
  class CSection
  {
  public:
    CStdString m_strSectionName;
    long m_lReferenceCount;
  };
  class CDll
  {
  public:
    CStdString m_strDllName;
    long m_lReferenceCount;
    DllLoader *m_pDll;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
  static DllLoader *LoadDLL(const CStdString& strSection);
  static void UnloadDLL(const CStdString& strSection);
  static void UnloadAll();
protected:
  vector<CSection> m_vecLoadedSections;
  typedef vector<CSection>::iterator ivecLoadedSections;
  vector<CDll> m_vecLoadedDLLs;
};
extern class CSectionLoader g_sectionLoader;
