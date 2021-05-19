/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDON_BASE_H
#define C_API_ADDON_BASE_H

#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include "stdbool.h"
#include "stdint.h"

#ifndef TARGET_WINDOWS
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

// Generic helper definitions for smallest possible alignment
//@{
#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#define ATTRIBUTE_PACKED __attribute__((packed))
#define PRAGMA_PACK 0
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif
//@}

// Generic helper definitions for inline function support
//@{
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
//@}

// Generic helper definitions for shared library support
//@{
#if defined _WIN32 || defined __CYGWIN__
#define ATTRIBUTE_DLL_IMPORT __declspec(dllimport)
#define ATTRIBUTE_DLL_EXPORT __declspec(dllexport)
#define ATTRIBUTE_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define ATTRIBUTE_DLL_IMPORT __attribute__ ((visibility ("default")))
#define ATTRIBUTE_DLL_EXPORT __attribute__ ((visibility ("default")))
#define ATTRIBUTE_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define ATTRIBUTE_DLL_IMPORT
#define ATTRIBUTE_DLL_EXPORT
#define ATTRIBUTE_DLL_LOCAL
#endif
#endif
#define ATTRIBUTE_HIDDEN ATTRIBUTE_DLL_LOCAL // Fallback to old
//@}

// Hardware specific device context interface
#define ADDON_HARDWARE_CONTEXT void*

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
  /// @ingroup cpp_kodi_addon_addonbase_Defs
  /// @defgroup cpp_kodi_addon_addonbase_Defs_ADDON_STATUS enum ADDON_STATUS
  /// @brief <b>Return value of functions in @ref cpp_kodi_addon_addonbase "kodi::addon::CAddonBase"
  /// and associated classes</b>\n
  /// With this Kodi can do any follow-up work or add-on e.g. declare it as defective.
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

  /*! @brief Standard undefined pointer handle */
  typedef void* KODI_HANDLE;

  /*!
   * @brief Handle used to return data from the PVR add-on to CPVRClient
   */
  struct ADDON_HANDLE_STRUCT
  {
    void* callerAddress; /*!< address of the caller */
    void* dataAddress; /*!< address to store data in */
    int dataIdentifier; /*!< parameter to pass back when calling the callback */
  };
  typedef struct ADDON_HANDLE_STRUCT* ADDON_HANDLE;

  /*!
   * @brief Callback function tables from addon to Kodi
   * Set complete from Kodi!
   */
  struct AddonToKodiFuncTable_kodi;
  struct AddonToKodiFuncTable_kodi_audioengine;
  struct AddonToKodiFuncTable_kodi_filesystem;
  struct AddonToKodiFuncTable_kodi_network;
  struct AddonToKodiFuncTable_kodi_gui;
  typedef struct AddonToKodiFuncTable_Addon
  {
    // Pointer inside Kodi, used on callback functions to give related handle
    // class, for this ADDON::CAddonDll inside Kodi.
    KODI_HANDLE kodiBase;

    // Function addresses used for callbacks from addon to Kodi
    char* (*get_type_version)(void* kodiBase, int type);

    void (*free_string)(void* kodiBase, char* str);
    void (*free_string_array)(void* kodiBase, char** arr, int numElements);
    char* (*get_addon_path)(void* kodiBase);
    char* (*get_base_user_path)(void* kodiBase);
    void (*addon_log_msg)(void* kodiBase, const int loglevel, const char* msg);

    bool (*get_setting_bool)(void* kodiBase, const char* id, bool* value);
    bool (*get_setting_int)(void* kodiBase, const char* id, int* value);
    bool (*get_setting_float)(void* kodiBase, const char* id, float* value);
    bool (*get_setting_string)(void* kodiBase, const char* id, char** value);

    bool (*set_setting_bool)(void* kodiBase, const char* id, bool value);
    bool (*set_setting_int)(void* kodiBase, const char* id, int value);
    bool (*set_setting_float)(void* kodiBase, const char* id, float value);
    bool (*set_setting_string)(void* kodiBase, const char* id, const char* value);

    void* (*get_interface)(void* kodiBase, const char* name, const char* version);

    struct AddonToKodiFuncTable_kodi* kodi;
    struct AddonToKodiFuncTable_kodi_audioengine* kodi_audioengine;
    struct AddonToKodiFuncTable_kodi_filesystem* kodi_filesystem;
    struct AddonToKodiFuncTable_kodi_gui* kodi_gui;
    struct AddonToKodiFuncTable_kodi_network* kodi_network;

    // Move up by min version change about
    bool (*is_setting_using_default)(void* kodiBase, const char* id);
  } AddonToKodiFuncTable_Addon;

  /*!
   * @brief Function tables from Kodi to addon
   */
  typedef struct KodiToAddonFuncTable_Addon
  {
    void (*destroy)();
    ADDON_STATUS (*get_status)(); // TODO unused remove by next min version increase
    ADDON_STATUS(*create_instance)
    (int instanceType,
     const char* instanceID,
     KODI_HANDLE instance,
     const char* version,
     KODI_HANDLE* addonInstance,
     KODI_HANDLE parent);
    void (*destroy_instance)(int instanceType, KODI_HANDLE instance);
    ADDON_STATUS (*set_setting)(const char* settingName, const void* settingValue);
  } KodiToAddonFuncTable_Addon;

  /*!
   * @brief Main structure passed from kodi to addon with basic information needed to
   * create add-on.
   */
  typedef struct AddonGlobalInterface
  {
    // String with full path where add-on is installed (without his name on end)
    // Set from Kodi!
    const char* libBasePath;

    // Master API version of Kodi itself (ADDON_GLOBAL_VERSION_MAIN)
    const char* kodi_base_api_version;

    // Pointer of first created instance, used in case this add-on goes with single way
    // Set from Kodi!
    KODI_HANDLE firstKodiInstance;

    // Pointer to master base class inside add-on
    // Set from addon header (kodi::addon::CAddonBase)!
    KODI_HANDLE addonBase;

    // Pointer to a instance used on single way (together with this class)
    // Set from addon header (kodi::addon::IAddonInstance)!
    KODI_HANDLE globalSingleInstance;

    // Callback function tables from addon to Kodi
    // Set from Kodi!
    AddonToKodiFuncTable_Addon* toKodi;

    // Function tables from Kodi to addon
    // Set from addon header!
    KodiToAddonFuncTable_Addon* toAddon;
  } AddonGlobalInterface;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDON_BASE_H */
