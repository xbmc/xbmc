#ifndef _WIN32DLLLOADER_H_
#define _WIN32DLLLOADER_H_

class Win32DllLoader : public LibraryLoader
{
public:
  Win32DllLoader(const char *dll);
  ~Win32DllLoader();
  
  virtual bool Load();
  virtual void Unload();
  
  virtual int ResolveExport(const char* symbol, void** ptr);
  virtual bool IsSystemDll();
  virtual HMODULE GetHModule();
  virtual bool HasSymbols();  
  
private:
  HMODULE m_dllHandle;
};

#endif