#pragma once

#include "cores/DllLoader/dll.h"

// dll defines
#define APE_DLL "Q:\\system\\players\\PAPlayer\\MACDll.dll"
#define SHN_DLL "Q:\\system\\players\\PAPlayer\\libshnplay.dll"
#define MPC_DLL "Q:\\system\\players\\PAPlayer\\MPCcodec.dll"
#define OGG_DLL "Q:\\system\\players\\PAPlayer\\vorbisfile.dll"
#define AAC_DLL "Q:\\system\\players\\paplayer\\aaccodec.dll"

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
    DWORD m_lUnloadDelayStartTick;
  };
  CSectionLoader(void);
  virtual ~CSectionLoader(void);

  static bool IsLoaded(const CStdString& strSection);
  static bool Load(const CStdString& strSection);
  static void Unload(const CStdString& strSection);
  static DllLoader *LoadDLL(const CStdString& strSection);
  static void UnloadDLL(const CStdString& strSection);
  static void UnloadDLLsDelayed();
  static void UnloadAll();
protected:
  vector<CSection> m_vecLoadedSections;
  typedef vector<CSection>::iterator ivecLoadedSections;
  vector<CDll> m_vecLoadedDLLs;
  CCriticalSection m_critSection;
};
extern class CSectionLoader g_sectionLoader;
