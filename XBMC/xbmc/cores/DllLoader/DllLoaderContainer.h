
#pragma once

#include "DllLoader.h"

#ifndef _WINDEF_
typedef unsigned long HMODULE;
#endif // _WINDEF_

class DllLoaderContainer
{
public:
  DllLoaderContainer();

#ifndef _LINUX
  DllLoader kernel32;
  DllLoader user32;
  DllLoader ddraw;
  DllLoader wininet;
  DllLoader advapi32;
  DllLoader ws2_32;
  DllLoader wsock32;
  DllLoader ole32;
  DllLoader oleaut32;
  DllLoader xbp; // exports for python dll
  DllLoader winmm;
  DllLoader msdmo;
  DllLoader xbmc_vobsub;
  DllLoader xbox_dx8;
  DllLoader version;
  DllLoader comdlg32;
  DllLoader gdi32;
  DllLoader comctl32;
  DllLoader msvcrt;
  DllLoader msvcr71;
  DllLoader pncrt;
  DllLoader iconvx;
#endif

  void Clear();
  HMODULE GetModuleAddress(const char* sName);
  int GetNrOfModules();
  DllLoader* GetModule(int iPos);
  DllLoader* GetModule(const char* sName);
  DllLoader* GetModule(HMODULE hModule);
  
  DllLoader* LoadModule(const char* sName, const char* sCurrentDir=NULL, bool bLoadSymbols=false);
  void ReleaseModule(DllLoader*& pDll);

  void RegisterDll(DllLoader* pDll);
  void UnRegisterDll(DllLoader* pDll);
  void UnloadPythonDlls();
private:
  DllLoader* FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols);
  DllLoader* LoadDll(const char* sName, bool bLoadSymbols);
  bool IsSystemDll(const char* sName);
  DllLoader* m_dlls[64];
  int m_iNrOfDlls;

  bool m_bTrack;
};

extern DllLoaderContainer g_dlls;
