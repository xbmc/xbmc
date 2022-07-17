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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
#undef ATTR_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#define ATTR_PACKED __attribute__((packed))
#define PRAGMA_PACK 0
#endif

#if !defined(ATTR_PACKED)
#define ATTR_PACKED
#define PRAGMA_PACK 1
#endif
//@}

// Generic helper definitions for inline function support
//@{
#ifdef _MSC_VER
#define ATTR_FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define ATTR_FORCEINLINE inline __attribute__((__always_inline__))
#elif defined(__CLANG__)
#if __has_attribute(__always_inline__)
#define ATTR_FORCEINLINE inline __attribute__((__always_inline__))
#else
#define ATTR_FORCEINLINE inline
#endif
#else
#define ATTR_FORCEINLINE inline
#endif
//@}

// Generic helper definitions for shared library support
//@{
#if defined _WIN32 || defined _WIN64 || defined __CYGWIN__
#define ATTR_DLL_IMPORT __declspec(dllimport)
#define ATTR_DLL_EXPORT __declspec(dllexport)
#define ATTR_DLL_LOCAL
#ifdef _WIN64
#define ATTR_APIENTRY __stdcall
#else
#define ATTR_APIENTRY __cdecl
#endif
#else
#if __GNUC__ >= 4
#define ATTR_DLL_IMPORT __attribute__((visibility("default")))
#define ATTR_DLL_EXPORT __attribute__((visibility("default")))
#ifndef SWIG
#define ATTR_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define ATTR_DLL_LOCAL
#endif
#else
#define ATTR_DLL_IMPORT
#define ATTR_DLL_EXPORT
#define ATTR_DLL_LOCAL
#endif
#define ATTR_APIENTRY
#endif

#ifndef ATTR_APIENTRYP
#define ATTR_APIENTRYP ATTR_APIENTRY*
#endif
//@}

#ifdef _WIN32 // windows
#if !defined(_SSIZE_T_DEFINED) && !defined(HAVE_SSIZE_T)
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#ifndef SSIZE_MAX
#define SSIZE_MAX INTPTR_MAX
#endif // !SSIZE_MAX
#else // Linux, Mac, FreeBSD
#include <sys/types.h>
#endif // TARGET_POSIX

/*
 * To have a on add-on and kodi itself handled string always on known size!
 */
#define ADDON_STANDARD_STRING_LENGTH 1024
#define ADDON_STANDARD_STRING_LENGTH_SMALL 256

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef void* KODI_ADDON_HDL;
  typedef void* KODI_ADDON_BACKEND_HDL;
  typedef void* KODI_ADDON_INSTANCE_HDL;
  typedef void* KODI_ADDON_INSTANCE_BACKEND_HDL;

  // Hardware specific device context interface
  typedef void* ADDON_HARDWARE_CONTEXT;

  typedef void* KODI_ADDON_FUNC_DUMMY;

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
  /// @defgroup cpp_kodi_Defs_ADDON_LOG enum ADDON_LOG
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
  typedef enum ADDON_LOG
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
  } ADDON_LOG;
  ///@}
  //----------------------------------------------------------------------------

  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_STRING_V1)(
      const KODI_ADDON_INSTANCE_HDL hdl, const char* name, const char* value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_BOOLEAN_V1)(
      const KODI_ADDON_INSTANCE_HDL hdl, const char* name, bool value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_INTEGER_V1)(
      const KODI_ADDON_INSTANCE_HDL hdl, const char* name, int value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_FLOAT_V1)(
      const KODI_ADDON_INSTANCE_HDL hdl, const char* name, float value);

  typedef struct KODI_ADDON_INSTANCE_FUNC
  {
    PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_STRING_V1 instance_setting_change_string;
    PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_BOOLEAN_V1 instance_setting_change_boolean;
    PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_INTEGER_V1 instance_setting_change_integer;
    PFN_KODI_ADDON_INSTANCE_SETTING_CHANGE_FLOAT_V1 instance_setting_change_float;
  } KODI_ADDON_INSTANCE_FUNC;

  typedef struct KODI_ADDON_INSTANCE_FUNC_CB
  {
    char* (*get_instance_user_path)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl);
    bool (*is_instance_setting_using_default)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                              const char* id);

    bool (*get_instance_setting_bool)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                      const char* id,
                                      bool* value);
    bool (*get_instance_setting_int)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                     const char* id,
                                     int* value);
    bool (*get_instance_setting_float)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                       const char* id,
                                       float* value);
    bool (*get_instance_setting_string)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                        const char* id,
                                        char** value);

    bool (*set_instance_setting_bool)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                      const char* id,
                                      bool value);
    bool (*set_instance_setting_int)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                     const char* id,
                                     int value);
    bool (*set_instance_setting_float)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                       const char* id,
                                       float value);
    bool (*set_instance_setting_string)(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                        const char* id,
                                        const char* value);
  } KODI_ADDON_INSTANCE_FUNC_CB;

  typedef int KODI_ADDON_INSTANCE_TYPE;

  typedef struct KODI_ADDON_INSTANCE_INFO
  {
    KODI_ADDON_INSTANCE_TYPE type;
    uint32_t number;
    const char* id;
    const char* version;
    KODI_ADDON_INSTANCE_BACKEND_HDL kodi;
    KODI_ADDON_INSTANCE_HDL parent;
    bool first_instance;

    struct KODI_ADDON_INSTANCE_FUNC_CB* functions;
  } KODI_ADDON_INSTANCE_INFO;

  typedef struct KODI_ADDON_INSTANCE_STRUCT
  {
    const KODI_ADDON_INSTANCE_INFO* info;

    KODI_ADDON_INSTANCE_HDL hdl;
    struct KODI_ADDON_INSTANCE_FUNC* functions;
    union
    {
      KODI_ADDON_FUNC_DUMMY dummy;
      struct AddonInstance_AudioDecoder* audiodecoder;
      struct AddonInstance_AudioEncoder* audioencoder;
      struct AddonInstance_ImageDecoder* imagedecoder;
      struct AddonInstance_Game* game;
      struct AddonInstance_InputStream* inputstream;
      struct AddonInstance_Peripheral* peripheral;
      struct AddonInstance_PVR* pvr;
      struct AddonInstance_Screensaver* screensaver;
      struct AddonInstance_VFSEntry* vfs;
      struct AddonInstance_VideoCodec* videocodec;
      struct AddonInstance_Visualization* visualization;
    };
  } KODI_ADDON_INSTANCE_STRUCT;

  /*! @brief Standard undefined pointer handle */
  typedef void* KODI_HANDLE;

  typedef struct AddonToKodiFuncTable_kodi_addon
  {
    char* (*get_addon_path)(const KODI_ADDON_BACKEND_HDL hdl);
    char* (*get_lib_path)(const KODI_ADDON_BACKEND_HDL hdl);
    char* (*get_user_path)(const KODI_ADDON_BACKEND_HDL hdl);
    char* (*get_temp_path)(const KODI_ADDON_BACKEND_HDL hdl);

    char* (*get_localized_string)(const KODI_ADDON_BACKEND_HDL hdl, long label_id);

    bool (*open_settings_dialog)(const KODI_ADDON_BACKEND_HDL hdl);
    bool (*is_setting_using_default)(const KODI_ADDON_BACKEND_HDL hdl, const char* id);

    bool (*get_setting_bool)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool* value);
    bool (*get_setting_int)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int* value);
    bool (*get_setting_float)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, float* value);
    bool (*get_setting_string)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, char** value);

    bool (*set_setting_bool)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool value);
    bool (*set_setting_int)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int value);
    bool (*set_setting_float)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, float value);
    bool (*set_setting_string)(const KODI_ADDON_BACKEND_HDL hdl, const char* id, const char* value);

    char* (*get_addon_info)(const KODI_ADDON_BACKEND_HDL hdl, const char* id);

    char* (*get_type_version)(const KODI_ADDON_BACKEND_HDL hdl, int type);
    void* (*get_interface)(const KODI_ADDON_BACKEND_HDL hdl, const char* name, const char* version);
  } AddonToKodiFuncTable_kodi_addon;

  /*!
   * @brief Callback function tables from addon to Kodi
   * Set complete from Kodi!
   */
  typedef struct AddonToKodiFuncTable_Addon
  {
    // Pointer inside Kodi, used on callback functions to give related handle
    // class, for this ADDON::CAddonDll inside Kodi.
    KODI_ADDON_BACKEND_HDL kodiBase;

    void (*free_string)(const KODI_ADDON_BACKEND_HDL hdl, char* str);
    void (*free_string_array)(const KODI_ADDON_BACKEND_HDL hdl, char** arr, int numElements);
    void (*addon_log_msg)(const KODI_ADDON_BACKEND_HDL hdl, const int loglevel, const char* msg);

    struct AddonToKodiFuncTable_kodi* kodi;
    struct AddonToKodiFuncTable_kodi_addon* kodi_addon;
    struct AddonToKodiFuncTable_kodi_audioengine* kodi_audioengine;
    struct AddonToKodiFuncTable_kodi_filesystem* kodi_filesystem;
    struct AddonToKodiFuncTable_kodi_gui* kodi_gui;
    struct AddonToKodiFuncTable_kodi_network* kodi_network;
  } AddonToKodiFuncTable_Addon;

  typedef ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_CREATE_V1)(
      const KODI_ADDON_INSTANCE_BACKEND_HDL first_instance, KODI_ADDON_HDL* hdl);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_DESTROY_V1)(const KODI_ADDON_HDL hdl);
  typedef ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_CREATE_INSTANCE_V1)(
      const KODI_ADDON_HDL hdl, struct KODI_ADDON_INSTANCE_STRUCT* instance);
  typedef void(ATTR_APIENTRYP PFN_KODI_ADDON_DESTROY_INSTANCE_V1)(
      const KODI_ADDON_HDL hdl, struct KODI_ADDON_INSTANCE_STRUCT* instance);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_SETTING_CHANGE_STRING_V1)(
      const KODI_ADDON_HDL hdl, const char* name, const char* value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_SETTING_CHANGE_BOOLEAN_V1)(
      const KODI_ADDON_HDL hdl, const char* name, bool value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_SETTING_CHANGE_INTEGER_V1)(
      const KODI_ADDON_HDL hdl, const char* name, int value);
  typedef enum ADDON_STATUS(ATTR_APIENTRYP PFN_KODI_ADDON_SETTING_CHANGE_FLOAT_V1)(
      const KODI_ADDON_HDL hdl, const char* name, float value);

  /*!
   * @brief Function tables from Kodi to addon
   */
  typedef struct KodiToAddonFuncTable_Addon
  {
    PFN_KODI_ADDON_CREATE_V1 create;
    PFN_KODI_ADDON_DESTROY_V1 destroy;
    PFN_KODI_ADDON_CREATE_INSTANCE_V1 create_instance;
    PFN_KODI_ADDON_DESTROY_INSTANCE_V1 destroy_instance;
    PFN_KODI_ADDON_SETTING_CHANGE_STRING_V1 setting_change_string;
    PFN_KODI_ADDON_SETTING_CHANGE_BOOLEAN_V1 setting_change_boolean;
    PFN_KODI_ADDON_SETTING_CHANGE_INTEGER_V1 setting_change_integer;
    PFN_KODI_ADDON_SETTING_CHANGE_FLOAT_V1 setting_change_float;
  } KodiToAddonFuncTable_Addon;

  /*!
   * @brief Main structure passed from kodi to addon with basic information needed to
   * create add-on.
   */
  typedef struct AddonGlobalInterface
  {
    // Pointer of first created instance, used in case this add-on goes with single way
    // Set from Kodi!
    struct KODI_ADDON_INSTANCE_STRUCT* firstKodiInstance;

    // Pointer to master base class inside add-on
    // Set from addon header (kodi::addon::CAddonBase)!
    KODI_ADDON_HDL addonBase;

    // Pointer to a instance used on single way (together with this class)
    // Set from addon header (kodi::addon::IAddonInstance)!
    KODI_ADDON_INSTANCE_HDL globalSingleInstance;

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
