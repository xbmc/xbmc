#ifndef SO_LOADER
#define SO_LOADER

#include <stdio.h>
#include "system.h"
#include "PlatformDefs.h"
#include "DllLoader.h"

class SoLoader : public DllLoader
{
public:
  SoLoader(const char *so);
  ~SoLoader();
  
  virtual bool Load();
  virtual void Unload();
  
  virtual int ResolveExport(const char* symbol, void** f);
  
private:
  void* m_soHandle;  
};

#endif
