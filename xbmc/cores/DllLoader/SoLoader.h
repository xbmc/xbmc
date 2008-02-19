#ifndef SO_LOADER
#define SO_LOADER

#include <stdio.h>
#include "system.h"
#ifdef _LINUX
#include "PlatformDefs.h"
#endif
#include "DllLoader.h"

class SoLoader : public LibraryLoader
{
public:
  SoLoader(const char *so, bool bGlobal = false);
  ~SoLoader();
  
  virtual bool Load();
  virtual void Unload();
  
  virtual int ResolveExport(const char* symbol, void** ptr);
  virtual bool IsSystemDll();
  virtual HMODULE GetHModule();
  virtual bool HasSymbols();  
  
private:
  void* m_soHandle;
  bool m_bGlobal;
  bool m_bLoaded;
};

#endif
