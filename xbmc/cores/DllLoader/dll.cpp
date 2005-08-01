
#include "../../stdafx.h"
#include "dll.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"

#define DLL_PROCESS_DETACH   0
#define DLL_PROCESS_ATTACH   1
#define DLL_THREAD_ATTACH    2
#define DLL_THREAD_DETACH    3
#define DLL_PROCESS_VERIFIER 4
  
#define DEFAULT_DLLPATH "Q:\\system\\players\\mplayer\\codecs"

typedef BOOL WINAPI EntryFunc(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

char* getpath(char *buf, const char *full)
{
  char* pos;
  if (pos = strrchr(full, '\\'))
  {
    strncpy(buf, full, pos - full);
    buf[pos - full] = 0;
    return buf;
  }
  else return NULL;
}

extern "C" HMODULE __stdcall dllLoadLibraryExtended(LPCSTR file, LPCSTR sourcedll)
{
  // we skip to the last backslash
  // this is effectively eliminating weird characters in
  // the text output windows

  char* libname = strrchr(file, '\\');
  if (libname) libname++;
  else libname = (char*)file;

  char llibname[MAX_PATH + 1]; // lower case
  strcpy(llibname, libname);
  strlwr(llibname);

  CLog::Log(LOGDEBUG, "LoadLibraryA('%s')", libname);
  char* l = llibname;
  
    char pfile[MAX_PATH + 1];
  if (strlen(file) > 1 && file[1] == ':')
    sprintf(pfile, "%s", (char *)file);
  else
  {
    if (sourcedll)
    {
      //Use calling dll's path as base address for this call
      char path[MAX_PATH + 1];
      getpath(path, sourcedll);

      //Handle mplayer case specially
      //it has all it's dlls in a codecs subdirectory
      if (strstr(sourcedll, "mplayer.dll"))
        sprintf(pfile, "%s\\codecs\\%s", path, (char *)libname);
      else
        sprintf(pfile, "%s\\%s", path, (char *)libname);
    }
    else
      sprintf(pfile, "%s\\%s", DEFAULT_DLLPATH , (char *)libname);
  }
  
  // Check if dll is already loaded and return its handle
  DllLoader* dll = g_dlls.GetModule(pfile);
  if (dll)
  {
    if (!dll->IsSystemDll())
    {
      CLog::Log(LOGDEBUG, "%s already loaded -> 0x%x", dll->GetFileName(), dll);
      dll->IncrRef();
    }
    return (HMODULE)dll;
  }

  // enable memory tracking by default
  DllLoader * dllhandle = new DllLoader(pfile, true);

  int hr = dllhandle->Parse();
  if (hr == 0)
  {
    CLog::Log(LOGERROR, "Failed to open %s, check codecs.conf and file existence.\n", pfile);
    delete dllhandle;
    return NULL;
  }

  dllhandle->ResolveImports();

  // only execute DllMain if no EntryPoint is found
  if (!dllhandle->EntryAddress)
  {
    void* address = NULL;
    dllhandle->ResolveExport("DllMain", &address);
    if (address) dllhandle->EntryAddress = (unsigned long)address;
  }
  else
  {
    CLog::Log(LOGDEBUG, "Executing EntryPoint at: 0x%x - Dll: %s", dllhandle->EntryAddress, libname);
  }

  // patch some unwanted calls in memory
  if (strstr(libname, "QuickTime.qts") && dllhandle)
  {
    int i;
    DWORD dispatch_addr;
    DWORD imagebase_addr;
    DWORD dispatch_rva;

    dllhandle->ResolveExport("theQuickTimeDispatcher", (void **)&dispatch_addr);
    imagebase_addr = (DWORD)dllhandle->hModule;
    CLog::Log(LOGDEBUG, "Virtual Address of theQuickTimeDispatcher = 0x%x", dispatch_addr);
    CLog::Log(LOGDEBUG, "ImageBase of %s = 0x%x", libname, imagebase_addr);

    dispatch_rva = dispatch_addr - imagebase_addr;

    CLog::Log(LOGDEBUG, "Relative Virtual Address of theQuickTimeDispatcher = %p", dispatch_rva);

    DWORD base = imagebase_addr;
    if (dispatch_rva == 0x124C30)
    {
      CLog::Log(LOGINFO, "QuickTime5 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e842)[i] = 0x90; // make_new_region ?
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x19e86d)[i] = 0x90; // call__call_CreateCompatibleDC ?
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x19e898)[i] = 0x90; // jmp_to_call_loadbitmap ?
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x19e8ac)[i] = 0x90; // call__calls_OLE_shit ?
      for (i = 0;i < 106;i++) ((BYTE*)base + 0x261B10)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13B330)
    {
      CLog::Log(LOGINFO, "QuickTime6 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x2730CC)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x2730f7)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x273122)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x273131)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2AC852)[i] = 0x90; // disable threads
    }
    else if (dispatch_rva == 0x13C3E0)
    {
      CLog::Log(LOGINFO, "QuickTime6.3 DLLs found\n");
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268F6C)[i] = 0x90; // make_new_region
      for (i = 0;i < 28;i++) ((BYTE*)base + 0x268F97)[i] = 0x90; // call__call_CreateCompatibleDC
      for (i = 0;i < 5;i++) ((BYTE*)base + 0x268FC2)[i] = 0x90; // jmp_to_call_loadbitmap
      for (i = 0;i < 9;i++) ((BYTE*)base + 0x268FD1)[i] = 0x90; // call__calls_OLE_shit
      for (i = 0;i < 96;i++) ((BYTE*)base + 0x2B4722)[i] = 0x90; // disable threads
    }
    else
    {
      CLog::Log(LOGERROR, "Unsupported QuickTime version");
      //return 0;
    }

    CLog::Log(LOGINFO, "QuickTime.qts patched!!!\n");
  }

  EntryFunc* initdll = (EntryFunc *)dllhandle->EntryAddress;
  (*initdll)((HINSTANCE) dllhandle, DLL_PROCESS_ATTACH , 0); //call "DllMain" with DLL_PROCESS_ATTACH

  CLog::Log(LOGDEBUG, "LoadLibrary('%s') returning: 0x%x", libname, dllhandle);

// this is handled by the constructor of DllLoader
/*
  // Add dll to m_vecDlls
  g_dlls.RegisterDll(dllhandle);
*/
  return (HMODULE) dllhandle;
}

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file)
{
  return dllLoadLibraryExtended(file, NULL);
}

#define DONT_RESOLVE_DLL_REFERENCES   0x00000001
#define LOAD_LIBRARY_AS_DATAFILE      0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH 0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL  0x00000010

extern "C" HMODULE __stdcall dllLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
  char strFlags[64];
  strFlags[0] = '\0';

  if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) strcat(strFlags, "\n - DONT_RESOLVE_DLL_REFERENCES");
  if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL) strcat(strFlags, "\n - LOAD_IGNORE_CODE_AUTHZ_LEVEL");
  if (dwFlags & LOAD_LIBRARY_AS_DATAFILE) strcat(strFlags, "\n - LOAD_LIBRARY_AS_DATAFILE");
  if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) strcat(strFlags, "\n - LOAD_WITH_ALTERED_SEARCH_PATH");

  CLog::Log(LOGDEBUG, "LoadLibraryExA called with flags: %s", strFlags);
  
  return dllLoadLibraryExtended(lpLibFileName, NULL);
}

extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule)
{
  CLog::Log(LOGDEBUG, "FreeLibrary(0x%x)", hLibModule);

  DllLoader* dllhandle = (DllLoader*)hLibModule;
  
  // to make sure systems dlls are never deleted
  if (dllhandle->IsSystemDll()) return 1;
  
  if (dllhandle->DecrRef() > 0)
  {
    CLog::Log(LOGDEBUG, "Cannot FreeLibrary(%s), refcount > 0", dllhandle->GetFileName());
    return 0;
  }

  EntryFunc* initdll = (EntryFunc*)dllhandle->EntryAddress;
  
  //call "DllMain" with DLL_PROCESS_DETACH
  (*initdll)((HINSTANCE)dllhandle->hModule, DLL_PROCESS_DETACH , 0);

// this is handled by the destructor of DllLoader
/*
  //Remove dll
  g_dlls.UnRegisterDll(dllhandle);
*/
  if (dllhandle) delete dllhandle;
  return 1;
}

extern "C" FARPROC __stdcall dllGetProcAddress(HMODULE hModule, LPCSTR function)
{
  void* address = NULL;
  
  DllLoader* dll = (DllLoader*)hModule;
  dll->ResolveExport(function, &address);

  CLog::Log(LOGDEBUG, "%s!GetProcAddress(0x%x, '%s') => 0x%x", dll->GetName(), hModule, function, address);
  return (FARPROC)address;
}

extern "C" HMODULE WINAPI dllGetModuleHandleA(LPCSTR lpModuleName)
{
  /*
  If the file name extension is omitted, the default library extension .dll is appended.
  The file name string can include a trailing point character (.) to indicate that the module name has no extension.
  The string does not have to specify a path. When specifying a path, be sure to use backslashes (\), not forward slashes (/).
  The name is compared (case independently)
  If this parameter is NULL, GetModuleHandle returns a handle to the file used to create the calling process (.exe file).
  */
  char* strModuleName = new char[strlen(lpModuleName) + 5];
  strcpy(strModuleName, lpModuleName);

  if (strrchr(strModuleName, '.') == 0) strcat(strModuleName, ".dll");

  CLog::Log(LOGDEBUG, "GetModuleHandleA(%s) .. looking up", lpModuleName);

  HMODULE h = g_dlls.GetModuleAddress(strModuleName);
  if (h)
  {
    CLog::Log(LOGDEBUG, "GetModuleHandleA('%s') => 0x%x", lpModuleName, h);
    return h;
  }
 
  delete []strModuleName;

  return NULL;
}

extern "C" DWORD WINAPI dllGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  if (NULL == hModule)
  {
    strncpy(lpFilename, "xbmc.xbe", nSize);
    CLog::Log(LOGDEBUG, "GetModuleFileNameA(0x%x, 0x%x, %d) => '%s'\n",
              hModule, lpFilename, nSize, lpFilename);
    return 1;
  }

  char* sName = ((DllLoader*)hModule)->GetFileName();
  if (sName)
  {
    strncpy(lpFilename, sName, nSize);
    return 1;
  }
  
  return 0;
}