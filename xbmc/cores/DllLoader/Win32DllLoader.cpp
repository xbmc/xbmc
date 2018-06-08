/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Win32DllLoader.h"
#include "DllLoader.h"
#include "DllLoaderContainer.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/win32/CharsetConverter.h"

#include "dll_tracker_library.h"
#include "dll_tracker_file.h"
#include "exports/emu_msvcrt.h"

#include <limits>

extern "C" FARPROC WINAPI dllWin32GetProcAddress(HMODULE hModule, LPCSTR function);

//dllLoadLibraryA, dllFreeLibrary, dllGetProcAddress are from dllLoader,
//they are wrapper functions of COFF/PE32 loader.
extern "C" HMODULE WINAPI dllLoadLibraryA(LPCSTR libname);
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);

// our exports
Export win32_exports[] =
{
  { "LoadLibraryA",                                 -1, (void*)dllLoadLibraryA,                              (void*)track_LoadLibraryA },
  { "FreeLibrary",                                  -1, (void*)dllFreeLibrary,                               (void*)track_FreeLibrary },
// msvcrt
  { "_close",                     -1, (void*)dll_close,                     (void*)track_close},
  { "_lseek",                     -1, (void*)dll_lseek,                     NULL },
  { "_read",                      -1, (void*)dll_read,                      NULL },
  { "_write",                     -1, (void*)dll_write,                     NULL },
  { "_lseeki64",                  -1, (void*)dll_lseeki64,                  NULL },
  { "_open",                      -1, (void*)dll_open,                      (void*)track_open },
  { "fflush",                     -1, (void*)dll_fflush,                    NULL },
  { "fprintf",                    -1, (void*)dll_fprintf,                   NULL },
  { "fwrite",                     -1, (void*)dll_fwrite,                    NULL },
  { "putchar",                    -1, (void*)dll_putchar,                   NULL },
  { "_fstat",                     -1, (void*)dll_fstat,                     NULL },
  { "_mkdir",                     -1, (void*)dll_mkdir,                     NULL },
  { "_stat",                      -1, (void*)dll_stat,                      NULL },
  { "_fstat32",                   -1, (void*)dll_fstat,                     NULL },
  { "_stat32",                    -1, (void*)dll_stat,                      NULL },
  { "_findclose",                 -1, (void*)dll_findclose,                 NULL },
  { "_findfirst",                 -1, (void*)dll_findfirst,                 NULL },
  { "_findnext",                  -1, (void*)dll_findnext,                  NULL },
  { "_findfirst64i32",            -1, (void*)dll_findfirst64i32,            NULL },
  { "_findnext64i32",             -1, (void*)dll_findnext64i32,             NULL },
  { "fclose",                     -1, (void*)dll_fclose,                    (void*)track_fclose},
  { "feof",                       -1, (void*)dll_feof,                      NULL },
  { "fgets",                      -1, (void*)dll_fgets,                     NULL },
  { "fopen",                      -1, (void*)dll_fopen,                     (void*)track_fopen},
  { "fopen_s",                    -1, (void*)dll_fopen_s,                   NULL },
  { "putc",                       -1, (void*)dll_putc,                      NULL },
  { "fputc",                      -1, (void*)dll_fputc,                     NULL },
  { "fputs",                      -1, (void*)dll_fputs,                     NULL },
  { "fread",                      -1, (void*)dll_fread,                     NULL },
  { "fseek",                      -1, (void*)dll_fseek,                     NULL },
  { "ftell",                      -1, (void*)dll_ftell,                     NULL },
  { "getc",                       -1, (void*)dll_getc,                      NULL },
  { "fgetc",                      -1, (void*)dll_getc,                      NULL },
  { "rewind",                     -1, (void*)dll_rewind,                    NULL },
  { "vfprintf",                   -1, (void*)dll_vfprintf,                  NULL },
  { "fgetpos",                    -1, (void*)dll_fgetpos,                   NULL },
  { "fsetpos",                    -1, (void*)dll_fsetpos,                   NULL },
  { "_stati64",                   -1, (void*)dll_stati64,                   NULL },
  { "_stat64",                    -1, (void*)dll_stat64,                    NULL },
  { "_stat64i32",                 -1, (void*)dll_stat64i32,                 NULL },
  { "_fstati64",                  -1, (void*)dll_fstati64,                  NULL },
  { "_fstat64",                   -1, (void*)dll_fstat64,                   NULL },
  { "_fstat64i32",                -1, (void*)dll_fstat64i32,                NULL },
  { "_telli64",                   -1, (void*)dll_telli64,                   NULL },
  { "_tell",                      -1, (void*)dll_tell,                      NULL },
  { "_fileno",                    -1, (void*)dll_fileno,                    NULL },
  { "ferror",                     -1, (void*)dll_ferror,                    NULL },
  { "freopen",                    -1, (void*)dll_freopen,                   (void*)track_freopen},
  { "fscanf",                     -1, (void*)dll_fscanf,                    NULL },
  { "ungetc",                     -1, (void*)dll_ungetc,                    NULL },
  { "_fdopen",                    -1, (void*)dll_fdopen,                    NULL },
  { "clearerr",                   -1, (void*)dll_clearerr,                  NULL },
  // for debugging
  { "printf",                     -1, (void*)dllprintf,                     NULL },
  { "vprintf",                    -1, (void*)dllvprintf,                    NULL },
  { "perror",                     -1, (void*)dllperror,                     NULL },
  { "puts",                       -1, (void*)dllputs,                       NULL },
  // workarounds for non-win32 signals
  { "signal",                     -1, (void*)dll_signal,                    NULL },

  // libdvdnav + python need this (due to us using dll_putenv() to put stuff only?)
  { "getenv",                     -1, (void*)dll_getenv,                    NULL },
  { "_environ",                   -1, (void*)&dll__environ,                 NULL },
  { "_open_osfhandle",            -1, (void*)dll_open_osfhandle,            NULL },

  { NULL,                          -1, NULL,                                NULL }
};

Win32DllLoader::Win32DllLoader(const std::string& dll, bool isSystemDll)
  : LibraryLoader(dll)
  , bIsSystemDll(isSystemDll)
{
  m_dllHandle = NULL;
  DllLoaderContainer::RegisterDll(this);
}

Win32DllLoader::~Win32DllLoader()
{
  if (m_dllHandle)
    Unload();
  DllLoaderContainer::UnRegisterDll(this);
}

bool Win32DllLoader::Load()
{
  using namespace KODI::PLATFORM::WINDOWS;

  if (m_dllHandle != NULL)
    return true;

  std::string strFileName = GetFileName();
  auto strDllW = ToW(CSpecialProtocol::TranslatePath(strFileName));

#ifdef TARGET_WINDOWS_STORE
  // The path cannot be an absolute path or a relative path that contains ".." in the path.
  auto appPath = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path();
  size_t len = appPath.size();

  if (!appPath.empty() && wcsnicmp(appPath.c_str(), strDllW.c_str(), len) == 0)
  {
    if (strDllW.at(len) == '\\' || strDllW.at(len) == '/')
      len++;
    std::wstring relative = strDllW.substr(len);
    m_dllHandle = LoadPackagedLibrary(relative.c_str(), 0);
  }
  else
    m_dllHandle = LoadPackagedLibrary(strDllW.c_str(), 0);
#else
  m_dllHandle = LoadLibraryExW(strDllW.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif

  if (!m_dllHandle)
  {
    DWORD dw = GetLastError();
    wchar_t* lpMsgBuf = NULL;
    DWORD strLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&lpMsgBuf, 0, NULL);
    if (strLen == 0)
      strLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&lpMsgBuf, 0, NULL);

    if (strLen != 0)
    {
      auto strMessage = FromW(lpMsgBuf, strLen);
      CLog::Log(LOGERROR, "%s: Failed to load \"%s\" with error %lu: \"%s\"", __FUNCTION__, CSpecialProtocol::TranslatePath(strFileName).c_str(), dw, strMessage.c_str());
    }
    else
      CLog::Log(LOGERROR, "%s: Failed to load \"%s\" with error %lu", __FUNCTION__, CSpecialProtocol::TranslatePath(strFileName).c_str(), dw);

    LocalFree(lpMsgBuf);
    return false;
  }

  // handle functions that the dll imports
  if (NeedsHooking(strFileName.c_str()))
    OverrideImports(strFileName);

  return true;
}

void Win32DllLoader::Unload()
{
  // restore our imports
  RestoreImports();

  if (m_dllHandle)
  {
    if (!FreeLibrary(m_dllHandle))
       CLog::Log(LOGERROR, "%s Unable to unload %s", __FUNCTION__, GetName());
  }

  m_dllHandle = NULL;
}

int Win32DllLoader::ResolveExport(const char* symbol, void** f, bool logging)
{
  if (!m_dllHandle && !Load())
  {
    if (logging)
      CLog::Log(LOGWARNING, "%s - Unable to resolve: %s %s, reason: DLL not loaded", __FUNCTION__, GetName(), symbol);
    return 0;
  }

  void *s = GetProcAddress(m_dllHandle, symbol);

  if (!s)
  {
    if (logging)
      CLog::Log(LOGWARNING, "%s - Unable to resolve: %s %s", __FUNCTION__, GetName(), symbol);
    return 0;
  }

  *f = s;
  return 1;
}

bool Win32DllLoader::IsSystemDll()
{
  return bIsSystemDll;
}

HMODULE Win32DllLoader::GetHModule()
{
  return m_dllHandle;
}

bool Win32DllLoader::HasSymbols()
{
  return false;
}

void Win32DllLoader::OverrideImports(const std::string &dll)
{
  using KODI::PLATFORM::WINDOWS::ToW;
  auto strdllW = ToW(CSpecialProtocol::TranslatePath(dll));
  auto image_base = reinterpret_cast<BYTE*>(m_dllHandle);

  if (!image_base)
  {
    CLog::Log(LOGERROR, "%s - unable to GetModuleHandle for dll %s", __FUNCTION__, dll.c_str());
    return;
  }

  auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(image_base);
  auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(image_base + dos_header->e_lfanew); // e_lfanew = value at 0x3c

  auto imp_desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
    image_base + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

  if (!imp_desc)
  {
    CLog::Log(LOGERROR, "%s - unable to get import directory for dll %s", __FUNCTION__, dll.c_str());
    return;
  }

  // loop over all imported dlls
  for (int i = 0; imp_desc[i].Characteristics != 0; i++)
  {
    auto dllName = reinterpret_cast<char*>(image_base + imp_desc[i].Name);

    // check whether this is one of our dll's.
    if (NeedsHooking(dllName))
    {
      // this will do a loadlibrary on it, which should effectively make sure that it's hooked
      // Note that the library has obviously already been loaded by the OS (as it's implicitly linked)
      // so all this will do is insert our hook and make sure our DllLoaderContainer knows about it
      auto hModule = dllLoadLibraryA(dllName);
      if (hModule)
        m_referencedDlls.push_back(hModule);
    }

    PIMAGE_THUNK_DATA orig_first_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(image_base + imp_desc[i].OriginalFirstThunk);
    PIMAGE_THUNK_DATA first_thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(image_base + imp_desc[i].FirstThunk);

    // and then loop over all imported functions
    for (int j = 0; orig_first_thunk[j].u1.Function != 0; j++)
    {
      void *fixup = NULL;
      if (orig_first_thunk[j].u1.Function & 0x80000000)
        ResolveOrdinal(dllName, (orig_first_thunk[j].u1.Ordinal & 0x7fffffff), &fixup);
      else
      { // resolve by name
        PIMAGE_IMPORT_BY_NAME orig_imports_by_name = (PIMAGE_IMPORT_BY_NAME)(
          image_base + orig_first_thunk[j].u1.AddressOfData);

        ResolveImport(dllName, (char*)orig_imports_by_name->Name, &fixup);
      }/*
      if (!fixup)
      { // create a dummy function for tracking purposes
        PIMAGE_IMPORT_BY_NAME orig_imports_by_name = (PIMAGE_IMPORT_BY_NAME)(
          image_base + orig_first_thunk[j].u1.AddressOfData);
        fixup = CreateDummyFunction(dllName, (char*)orig_imports_by_name->Name);
      }*/
      if (fixup)
      {
        // save the old function
        Import import;
        import.table = &first_thunk[j].u1.Function;
        import.function = first_thunk[j].u1.Function;
        m_overriddenImports.push_back(import);

        DWORD old_prot = 0;

        // change to protection settings so we can write to memory area
        VirtualProtect((PVOID)&first_thunk[j].u1.Function, 4, PAGE_EXECUTE_READWRITE, &old_prot);

        // patch the address of function to point to our overridden version
        first_thunk[j].u1.Function = (uintptr_t)fixup;

        // reset to old settings
        VirtualProtect((PVOID)&first_thunk[j].u1.Function, 4, old_prot, &old_prot);
      }
    }
  }
}

bool Win32DllLoader::NeedsHooking(const char *dllName)
{
  if ( !StringUtils::EndsWithNoCase(dllName, "libdvdcss-2.dll")
  && !StringUtils::EndsWithNoCase(dllName, "libdvdnav.dll"))
    return false;

  LibraryLoader *loader = DllLoaderContainer::GetModule(dllName);
  if (loader)
  {
    // may have hooked this already (we can have repeats in the import table)
    for (unsigned int i = 0; i < m_referencedDlls.size(); i++)
    {
      if (loader->GetHModule() == m_referencedDlls[i])
        return false;
    }
  }
  return true;
}

void Win32DllLoader::RestoreImports()
{
  // first unhook any referenced dll's
  for (auto& module : m_referencedDlls)
    dllFreeLibrary(module);
  m_referencedDlls.clear();

  for (auto& import : m_overriddenImports)
  {
    // change to protection settings so we can write to memory area
    DWORD old_prot = 0;
    VirtualProtect(import.table, 4, PAGE_EXECUTE_READWRITE, &old_prot);

    *static_cast<uintptr_t *>(import.table) = import.function;

    // reset to old settings
    VirtualProtect(import.table, 4, old_prot, &old_prot);
  }
}

bool FunctionNeedsWrapping(Export *exports, const char *functionName, void **fixup)
{
  Export *exp = exports;
  while (exp->name)
  {
    if (strcmp(exp->name, functionName) == 0)
    { //! @todo Should we be tracking stuff?
      if (0)
        *fixup = exp->track_function;
      else
        *fixup = exp->function;
      return true;
    }
    exp++;
  }
  return false;
}

bool Win32DllLoader::ResolveImport(const char *dllName, const char *functionName, void **fixup)
{
  return FunctionNeedsWrapping(win32_exports, functionName, fixup);
}

bool Win32DllLoader::ResolveOrdinal(const char *dllName, unsigned long ordinal, void **fixup)
{
  Export *exp = win32_exports;
  while (exp->name)
  {
    if (exp->ordinal == ordinal)
    { //! @todo Should we be tracking stuff?
      if (0)
        *fixup = exp->track_function;
      else
        *fixup = exp->function;
      return true;
    }
    exp++;
  }
  return false;
}

extern "C" FARPROC __stdcall dllWin32GetProcAddress(HMODULE hModule, LPCSTR function)
{
  // if the high-order word is zero, then lpProcName is the function's ordinal value
  if (reinterpret_cast<uintptr_t>(function) > std::numeric_limits<WORD>::max())
  {
    // first check whether this function is one of the ones we need to wrap
    void *fixup = NULL;
    if (FunctionNeedsWrapping(win32_exports, function, &fixup))
      return (FARPROC)fixup;
  }

  // Nope
  return GetProcAddress(hModule, function);
}

