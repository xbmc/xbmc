#pragma once

//  forward
class DllLoader;

class CSectionLoader
{
public:
  class CSection
  {
  public:
    CStdString m_strSectionName;
    long m_lReferenceCount;
    DWORD m_lUnloadDelayStartTick;
  };
  class CDll
  {
  public:
    CStdString m_strDllName;
    long m_lReferenceCount;
    DllLoader *m_pDll;
    DWORD m_lUnloadDelayStartTick;
    bool m_bDelayUnload;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
  static DllLoader* LoadDLL(const CStdString& strSection, bool bDelayUnload=true);
  static void UnloadDLL(const CStdString& strSection);
  static void UnloadDelayed();
  static void UnloadAll();
protected:
  vector<CSection> m_vecLoadedSections;
  typedef vector<CSection>::iterator ivecLoadedSections;
  vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;
};
extern class CSectionLoader g_sectionLoader;
