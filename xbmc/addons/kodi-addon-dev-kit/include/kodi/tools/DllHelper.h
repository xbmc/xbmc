/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <kodi/AddonBase.h>

#ifdef _WIN32                   // windows
#include <p8-platform/windows/dlfcn-win32.h>
#else
#include <dlfcn.h>              // linux+osx
#endif

#define REGISTER_DLL_SYMBOL(functionPtr) \
  CDllHelper::RegisterSymbol(functionPtr, #functionPtr)

/// @brief Class to help with load of shared library functions
///
/// You can add them as parent to your class and to help with load of shared
/// library functions.
///
/// @note To use on Windows must you also include p8-platform on your addon!
///
///
/// ----------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/tools/DllHelper.h>
///
/// ...
/// class CMyInstance : public kodi::addon::CInstanceAudioDecoder,
///                     private CDllHelper
/// {
/// public:
///   CMyInstance(KODI_HANDLE instance);
///   bool Start();
///
///   ...
///
///   /* The pointers for on shared library exported functions */
///   int (*Init)();
///   void (*Cleanup)();
///   int (*GetLength)();
/// };
///
/// CMyInstance::CMyInstance(KODI_HANDLE instance)
///   : CInstanceAudioDecoder(instance)
/// {
/// }
///
/// bool CMyInstance::Start()
/// {
///   std::string lib = kodi::GetAddonPath("myLib.so");
///   if (!LoadDll(lib)) return false;
///   if (!REGISTER_DLL_SYMBOL(Init)) return false;
///   if (!REGISTER_DLL_SYMBOL(Cleanup)) return false;
///   if (!REGISTER_DLL_SYMBOL(GetLength)) return false;
///
///   Init();
///   return true;
/// }
/// ...
/// ~~~~~~~~~~~~~
///
class CDllHelper
{
public:
  CDllHelper() : m_dll(nullptr) { }
  virtual ~CDllHelper()
  {
    if (m_dll)
      dlclose(m_dll);
  }

  /// @brief Function to load requested library
  ///
  /// @param[in] path         The path with filename of shared library to load
  /// @return                 true if load was successful done
  ///
  bool LoadDll(const std::string& path)
  {
    m_dll = dlopen(path.c_str(), RTLD_LAZY);
    if (m_dll == nullptr)
    {
      kodi::Log(ADDON_LOG_ERROR, "Unable to load %s", dlerror());
      return false;
    }
    return true;
  }

  /// @brief Function to register requested library symbol
  ///
  /// @note This function should not be used, use instead the macro
  /// REGISTER_DLL_SYMBOL to register the symbol pointer.
  ///
  template <typename T>
  bool RegisterSymbol(T& functionPtr, const char* strFunctionPtr)
  {
    functionPtr = reinterpret_cast<T>(dlsym(m_dll, strFunctionPtr));
    if (functionPtr == nullptr)
    {
      kodi::Log(ADDON_LOG_ERROR, "Unable to assign function %s", dlerror());
      return false;
    }
    return true;
  }

private:
  void* m_dll;
};
