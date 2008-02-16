#pragma once

//  forward
class LibraryLoader;

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
    LibraryLoader *m_pDll;
    DWORD m_lUnloadDelayStartTick;
    bool m_bDelayUnload;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
  static LibraryLoader* LoadDLL(const CStdString& strSection, bool bDelayUnload=true, bool bLoadSymbols=false);
  static void UnloadDLL(const CStdString& strSection);
  static void UnloadDelayed();
  static void UnloadAll();
protected:
  std::vector<CSection> m_vecLoadedSections;
  typedef std::vector<CSection>::iterator ivecLoadedSections;
  std::vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;
};
extern class CSectionLoader g_sectionLoader;
