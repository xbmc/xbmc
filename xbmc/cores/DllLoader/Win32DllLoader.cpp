/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32DllLoader.h"

#include "DllLoaderContainer.h"
#include "exports/emu_msvcrt.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <limits>
#include <string_view>

extern "C" FARPROC WINAPI dllWin32GetProcAddress(HMODULE hModule, LPCSTR function);

//dllLoadLibraryA, dllFreeLibrary are from dllLoader,
//they are wrapper functions of COFF/PE32 loader.
extern "C" HMODULE WINAPI dllLoadLibraryA(LPCSTR libname);
extern "C" BOOL WINAPI dllFreeLibrary(HINSTANCE hLibModule);

struct Export
{
  std::string_view name;
  unsigned long ordinal;
  void* function;
};

// our exports
// clang-format off
Export win32_exports[] =
{
  { "LoadLibraryA",               -1UL, (void*)dllLoadLibraryA,   },
  { "FreeLibrary",                -1UL, (void*)dllFreeLibrary,    },
// msvcrt
  { "_close",                     -1UL, (void*)dll_close          },
  { "_lseek",                     -1UL, (void*)dll_lseek          },
  { "_read",                      -1UL, (void*)dll_read           },
  { "_write",                     -1UL, (void*)dll_write          },
  { "_lseeki64",                  -1UL, (void*)dll_lseeki64       },
  { "_open",                      -1UL, (void*)dll_open           },
  { "fflush",                     -1UL, (void*)dll_fflush         },
  { "fprintf",                    -1UL, (void*)dll_fprintf        },
  { "fwrite",                     -1UL, (void*)dll_fwrite         },
  { "putchar",                    -1UL, (void*)dll_putchar        },
  { "_fstat",                     -1UL, (void*)dll_fstat          },
  { "_mkdir",                     -1UL, (void*)dll_mkdir          },
  { "_stat",                      -1UL, (void*)dll_stat           },
  { "_fstat32",                   -1UL, (void*)dll_fstat          },
  { "_stat32",                    -1UL, (void*)dll_stat           },
  { "_findclose",                 -1UL, (void*)dll_findclose      },
  { "_findfirst",                 -1UL, (void*)dll_findfirst      },
  { "_findnext",                  -1UL, (void*)dll_findnext       },
  { "_findfirst64i32",            -1UL, (void*)dll_findfirst64i32 },
  { "_findnext64i32",             -1UL, (void*)dll_findnext64i32  },
  { "fclose",                     -1UL, (void*)dll_fclose         },
  { "feof",                       -1UL, (void*)dll_feof           },
  { "fgets",                      -1UL, (void*)dll_fgets          },
  { "fopen",                      -1UL, (void*)dll_fopen          },
  { "fopen_s",                    -1UL, (void*)dll_fopen_s        },
  { "putc",                       -1UL, (void*)dll_putc           },
  { "fputc",                      -1UL, (void*)dll_fputc          },
  { "fputs",                      -1UL, (void*)dll_fputs          },
  { "fread",                      -1UL, (void*)dll_fread          },
  { "fseek",                      -1UL, (void*)dll_fseek          },
  { "ftell",                      -1UL, (void*)dll_ftell          },
  { "getc",                       -1UL, (void*)dll_getc           },
  { "fgetc",                      -1UL, (void*)dll_getc           },
  { "rewind",                     -1UL, (void*)dll_rewind         },
  { "vfprintf",                   -1UL, (void*)dll_vfprintf       },
  { "fgetpos",                    -1UL, (void*)dll_fgetpos        },
  { "fsetpos",                    -1UL, (void*)dll_fsetpos        },
  { "_stati64",                   -1UL, (void*)dll_stati64        },
  { "_stat64",                    -1UL, (void*)dll_stat64         },
  { "_stat64i32",                 -1UL, (void*)dll_stat64i32      },
  { "_fstati64",                  -1UL, (void*)dll_fstati64       },
  { "_fstat64",                   -1UL, (void*)dll_fstat64        },
  { "_fstat64i32",                -1UL, (void*)dll_fstat64i32     },
  { "_telli64",                   -1UL, (void*)dll_telli64        },
  { "_tell",                      -1UL, (void*)dll_tell           },
  { "_fileno",                    -1UL, (void*)dll_fileno         },
  { "ferror",                     -1UL, (void*)dll_ferror         },
  { "freopen",                    -1UL, (void*)dll_freopen        },
  { "fscanf",                     -1UL, (void*)dll_fscanf         },
  { "ungetc",                     -1UL, (void*)dll_ungetc         },
  { "_fdopen",                    -1UL, (void*)dll_fdopen         },
  { "clearerr",                   -1UL, (void*)dll_clearerr       },
  // for debugging
  { "printf",                     -1UL, (void*)dllprintf          },
  { "vprintf",                    -1UL, (void*)dllvprintf         },
  { "perror",                     -1UL, (void*)dllperror          },
  { "puts",                       -1UL, (void*)dllputs            },
  // workarounds for non-win32 signals
  { "signal",                     -1UL, (void*)dll_signal         },

  // libdvdnav + python need this (due to us using dll_putenv() to put stuff only?)
  { "getenv",                     -1UL, (void*)dll_getenv         },
  { "_environ",                   -1UL, (void*)&dll__environ      },
  { "_open_osfhandle",            -1UL, (void*)dll_open_osfhandle },
// clang-format off
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
      CLog::Log(LOGERROR, "{}: Failed to load \"{}\" with error {}: \"{}\"", __FUNCTION__,
                CSpecialProtocol::TranslatePath(strFileName), dw, strMessage);
    }
    else
      CLog::Log(LOGERROR, "{}: Failed to load \"{}\" with error {}", __FUNCTION__,
                CSpecialProtocol::TranslatePath(strFileName), dw);

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
      CLog::Log(LOGERROR, "{} Unable to unload {}", __FUNCTION__, GetName());
  }

  m_dllHandle = NULL;
}

int Win32DllLoader::ResolveExport(const char* symbol, void** f, bool logging)
{
  if (!m_dllHandle && !Load())
  {
    if (logging)
      CLog::Log(LOGWARNING, "{} - Unable to resolve: {} {}, reason: DLL not loaded", __FUNCTION__,
                GetName(), symbol);
    return 0;
  }

  void *s = GetProcAddress(m_dllHandle, symbol);

  if (!s)
  {
    if (logging)
      CLog::Log(LOGWARNING, "{} - Unable to resolve: {} {}", __FUNCTION__, GetName(), symbol);
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
    CLog::Log(LOGERROR, "{} - unable to GetModuleHandle for dll {}", __FUNCTION__, dll);
    return;
  }

  auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(image_base);
  auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(image_base + dos_header->e_lfanew); // e_lfanew = value at 0x3c

  auto imp_desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
    image_base + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

  if (!imp_desc)
  {
    CLog::Log(LOGERROR, "{} - unable to get import directory for dll {}", __FUNCTION__, dll);
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

bool FunctionNeedsWrapping(const char *functionName, void **fixup)
{
  for (const auto& exp : win32_exports)
  {
    if (exp.name == functionName)
    {
      *fixup = exp.function;
      return true;
    }
  }

  return false;
}

bool Win32DllLoader::ResolveImport(const char *dllName, const char *functionName, void **fixup)
{
  return FunctionNeedsWrapping(functionName, fixup);
}

bool Win32DllLoader::ResolveOrdinal(const char *dllName, unsigned long ordinal, void **fixup)
{
  for (const auto& exp : win32_exports)
  {
    if (exp.ordinal == ordinal)
    {
      *fixup = exp.function;
      return true;
    }
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
    if (FunctionNeedsWrapping(function, &fixup))
      return (FARPROC)fixup;
  }

  // Nope
  return GetProcAddress(hModule, function);
}

