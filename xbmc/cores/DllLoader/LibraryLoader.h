#ifndef LIBRARY_LOADER
#define LIBRARY_LOADER

#ifndef _LINUX
#ifndef _WINDEF_
typedef unsigned long HMODULE;
#endif // _WINDEF_
#else
#include "PlatformDefs.h"
#endif

class LibraryLoader
{
public:
  LibraryLoader(const char* libraryFile);
  virtual ~LibraryLoader();
  
  virtual bool Load() = 0;
  virtual void Unload() = 0;
  
  virtual int ResolveExport(const char* symbol, void** ptr) = 0;
  virtual bool IsSystemDll() = 0;
  virtual HMODULE GetHModule() = 0;
  virtual bool HasSymbols() = 0;
    
  char* GetName(); // eg "mplayer.dll"
  char* GetFileName(); // "Q:\system\mplayer\players\mplayer.dll"
  char* GetPath(); // "Q:\system\mplayer\players\"
  
  int IncrRef();
  int DecrRef();
  int GetRef();
  
private:
  char* m_sFileName;
  char* m_sPath;
  int   m_iRefCount;
};

#endif
