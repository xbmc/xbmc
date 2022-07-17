/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#include <string>

#include <dlfcn.h>
#include <kodi/AddonBase.h>
#include <kodi/Filesystem.h>

//==============================================================================
/// @ingroup cpp_kodi_tools_CDllHelper
/// @brief Macro to translate the given pointer value name of functions to
/// requested function name.
///
/// @note This should always be used and does the work of
/// @ref kodi::tools::CDllHelper::RegisterSymbol().
///
#define REGISTER_DLL_SYMBOL(functionPtr) \
  kodi::tools::CDllHelper::RegisterSymbol(functionPtr, #functionPtr)
//------------------------------------------------------------------------------

namespace kodi
{
namespace tools
{

//==============================================================================
/// @defgroup cpp_kodi_tools_CDllHelper class CDllHelper
/// @ingroup cpp_kodi_tools
/// @brief **Class to help with load of shared library functions**\n
/// You can add them as parent to your class and to help with load of shared
/// library functions.
///
/// @note To use on Windows must you also include [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32)
/// on your addon!\n\n
/// Furthermore, this allows the use of Android where the required library is
/// copied to an EXE useable folder.
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
///                     private kodi::tools::CDllHelper
/// {
/// public:
///   CMyInstance(KODI_HANDLE instance, const std::string& kodiVersion);
///   bool Start();
///
///   ...
///
///   // The pointers for on shared library exported functions
///   int (*Init)();
///   void (*Cleanup)();
///   int (*GetLength)();
/// };
///
/// CMyInstance::CMyInstance(KODI_HANDLE instance, const std::string& kodiVersion)
///   : CInstanceAudioDecoder(instance, kodiVersion)
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
///@{
class ATTR_DLL_LOCAL CDllHelper
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_tools_CDllHelper
  /// @brief Class constructor.
  ///
  CDllHelper() = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CDllHelper
  /// @brief Class destructor.
  ///
  virtual ~CDllHelper()
  {
    if (m_dll)
      dlclose(m_dll);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CDllHelper
  /// @brief Function to load requested library.
  ///
  /// @param[in] path The path with filename of shared library to load
  /// @return true if load was successful done
  ///
  bool LoadDll(std::string path)
  {
#if defined(TARGET_ANDROID)
    if (kodi::vfs::FileExists(path))
    {
      // Check already defined for "xbmcaltbinaddons", if yes no copy necassary.
      std::string xbmcaltbinaddons =
          kodi::vfs::TranslateSpecialProtocol("special://xbmcaltbinaddons/");
      if (path.compare(0, xbmcaltbinaddons.length(), xbmcaltbinaddons) != 0)
      {
        bool doCopy = true;
        std::string dstfile = xbmcaltbinaddons + kodi::vfs::GetFileName(path);

        kodi::vfs::FileStatus dstFileStat;
        if (kodi::vfs::StatFile(dstfile, dstFileStat))
        {
          kodi::vfs::FileStatus srcFileStat;
          if (kodi::vfs::StatFile(path, srcFileStat))
          {
            if (dstFileStat.GetSize() == srcFileStat.GetSize() &&
                dstFileStat.GetModificationTime() > srcFileStat.GetModificationTime())
              doCopy = false;
          }
        }

        if (doCopy)
        {
          kodi::Log(ADDON_LOG_DEBUG, "Caching '%s' to '%s'", path.c_str(), dstfile.c_str());
          if (!kodi::vfs::CopyFile(path, dstfile))
          {
            kodi::Log(ADDON_LOG_ERROR, "Failed to cache '%s' to '%s'", path.c_str(),
                      dstfile.c_str());
            return false;
          }
        }

        path = dstfile;
      }
    }
    else
    {
      return false;
    }
#endif

    m_dll = dlopen(path.c_str(), RTLD_LOCAL | RTLD_LAZY);
    if (m_dll == nullptr)
    {
      kodi::Log(ADDON_LOG_ERROR, "Unable to load %s", dlerror());
      return false;
    }
    return true;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_tools_CDllHelper
  /// @brief Function to register requested library symbol.
  ///
  /// @warning This function should not be used, use instead the macro
  /// @ref REGISTER_DLL_SYMBOL to register the symbol pointer.
  ///
  ///
  /// Use this always via Macro, e.g.:
  /// ~~~~~~~~~~~~~{.cpp}
  /// if (!REGISTER_DLL_SYMBOL(Init))
  ///   return false;
  /// ~~~~~~~~~~~~~
  ///
  template<typename T>
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
  //----------------------------------------------------------------------------

private:
  void* m_dll = nullptr;
};
///@}
//------------------------------------------------------------------------------

} /* namespace tools */
} /* namespace kodi */

#endif /* __cplusplus */
