
#pragma once

#include "DllLoader.h"

#ifndef _WINDEF_
typedef unsigned long HMODULE;
#endif // _WINDEF_

class DllLoaderContainer
{
public:  
  static void       Clear();
  static HMODULE    GetModuleAddress(const char* sName);
  static int        GetNrOfModules();
  static DllLoader* GetModule(int iPos);
  static DllLoader* GetModule(const char* sName);
  static DllLoader* GetModule(HMODULE hModule);  
  static DllLoader* LoadModule(const char* sName, const char* sCurrentDir=NULL, bool bLoadSymbols=false);
  static void       ReleaseModule(DllLoader*& pDll);

  static void RegisterDll(DllLoader* pDll);
  static void UnRegisterDll(DllLoader* pDll);
  static void UnloadPythonDlls();

private:
  static DllLoader* FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols);
  static DllLoader* LoadDll(const char* sName, bool bLoadSymbols);
  static bool       IsSystemDll(const char* sName);

  static DllLoader* m_dlls[64];
  static int m_iNrOfDlls;
  static bool m_bTrack;
};
