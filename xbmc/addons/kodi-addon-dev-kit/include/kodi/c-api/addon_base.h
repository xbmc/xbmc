/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "stdint.h"

#ifndef TARGET_WINDOWS
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#define ATTRIBUTE_PACKED __attribute__((packed))
#define PRAGMA_PACK 0
#define ATTRIBUTE_HIDDEN __attribute__((visibility("hidden")))
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#if !defined(ATTRIBUTE_HIDDEN)
#define ATTRIBUTE_HIDDEN
#endif

#ifdef _MSC_VER
#define ATTRIBUTE_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define ATTRIBUTE_FORCEINLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define ATTRIBUTE_FORCEINLINE inline __attribute__((__always_inline__))
#else
#define ATTRIBUTE_FORCEINLINE inline
#endif
#else
#define ATTRIBUTE_FORCEINLINE inline
#endif

/*
 * To have a on add-on and kodi itself handled string always on known size!
 */
#define ADDON_STANDARD_STRING_LENGTH 1024
#define ADDON_STANDARD_STRING_LENGTH_SMALL 256

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_addonbase
  /// @brief Return value of functions in @ref cpp_kodi_addon_addonbase "kodi::addon::CAddonBase"
  /// and associated classes.
  ///
  ///@{
  typedef enum ADDON_STATUS
  {
    /// @brief For everything OK and no error
    ADDON_STATUS_OK,

    /// @brief A needed connection was lost
    ADDON_STATUS_LOST_CONNECTION,

    /// @brief Addon needs a restart inside Kodi
    ADDON_STATUS_NEED_RESTART,

    /// @brief Necessary settings are not yet set
    ADDON_STATUS_NEED_SETTINGS,

    /// @brief Unknown and incomprehensible error
    ADDON_STATUS_UNKNOWN,

    /// @brief Permanent failure, like failing to resolve methods
    ADDON_STATUS_PERMANENT_FAILURE,

    /* internal used return error if function becomes not used from child on
    * addon */
    ADDON_STATUS_NOT_IMPLEMENTED
  } ADDON_STATUS;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_Defs_AddonLog enum AddonLog
  /// @ingroup cpp_kodi_Defs
  /// @brief **Log file type definitions**\n
  /// These define the types of log entries given with @ref kodi::Log() to Kodi.
  ///
  /// -------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/General.h>
  ///
  /// kodi::Log(ADDON_LOG_ERROR, "%s: There is an error occurred!", __func__);
  ///
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum AddonLog
  {
    /// @brief **0** : To include debug information in the log file.
    ADDON_LOG_DEBUG = 0,

    /// @brief **1** : To include information messages in the log file.
    ADDON_LOG_INFO = 1,

    /// @brief **2** : To write warnings in the log file.
    ADDON_LOG_WARNING = 2,

    /// @brief **3** : To report error messages in the log file.
    ADDON_LOG_ERROR = 3,

    /// @brief **4** : To notify fatal unrecoverable errors, which can may also indicate
    /// upcoming crashes.
    ADDON_LOG_FATAL = 4
  } AddonLog;
  ///@}
  //----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif /* __cplusplus */
