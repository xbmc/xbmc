/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <assert.h> /* assert */
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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
  #define ATTRIBUTE_PACKED __attribute__ ((packed))
  #define PRAGMA_PACK 0
  #define ATTRIBUTE_HIDDEN __attribute__ ((visibility ("hidden")))
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

#include "versions.h"

namespace kodi { namespace addon { class CAddonBase; }}
namespace kodi { namespace addon { class IAddonInstance; }}
namespace kodi { namespace gui { struct IRenderHelper; }}

extern "C" {

//==============================================================================
/// Standard undefined pointer handle
typedef void* KODI_HANDLE;
//------------------------------------------------------------------------------

//==============================================================================
///
/// @ingroup cpp_kodi_addon_addonbase
/// @brief Return value of functions in \ref kodi::addon::CAddonBase and
/// associated classes
///
typedef enum ADDON_STATUS
{
  /// For everything OK and no error
  ADDON_STATUS_OK,

  /// A needed connection was lost
  ADDON_STATUS_LOST_CONNECTION,

  /// Addon needs a restart inside Kodi
  ADDON_STATUS_NEED_RESTART,

  /// Necessary settings are not yet set
  ADDON_STATUS_NEED_SETTINGS,

  /// Unknown and incomprehensible error
  ADDON_STATUS_UNKNOWN,

  /// Permanent failure, like failing to resolve methods
  ADDON_STATUS_PERMANENT_FAILURE,

  /* internal used return error if function becomes not used from child on
   * addon */
  ADDON_STATUS_NOT_IMPLEMENTED
} ADDON_STATUS;
//------------------------------------------------------------------------------

//==============================================================================
/// @todo remove start with ADDON_* after old way on libXBMC_addon.h is removed
///
typedef enum AddonLog
{
  ///
  ADDON_LOG_DEBUG = 0,

  ///
  ADDON_LOG_INFO = 1,

  ///
  ADDON_LOG_NOTICE = 2,

  ///
  ADDON_LOG_WARNING = 3,

  ///
  ADDON_LOG_ERROR = 4,

  ///
  ADDON_LOG_SEVERE = 5,

  ///
  ADDON_LOG_FATAL = 6
} AddonLog;
//------------------------------------------------------------------------------

/*!
 * @brief Handle used to return data from the PVR add-on to CPVRClient
 */
struct ADDON_HANDLE_STRUCT
{
  void *callerAddress;  /*!< address of the caller */
  void *dataAddress;    /*!< address to store data in */
  int   dataIdentifier; /*!< parameter to pass back when calling the callback */
};
typedef ADDON_HANDLE_STRUCT *ADDON_HANDLE;

/*
 * To have a on add-on and kodi itself handled string always on known size!
 */
#define ADDON_STANDARD_STRING_LENGTH 1024
#define ADDON_STANDARD_STRING_LENGTH_SMALL 256

/*
 * Callback function tables from addon to Kodi
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
  void (*free_string)(void* kodiBase, char* str);
  void (*free_string_array)(void* kodiBase, char** arr, int numElements);
  char* (*get_addon_path)(void* kodiBase);
  char* (*get_base_user_path)(void* kodiBase);
  void (*addon_log_msg)(void* kodiBase, const int loglevel, const char *msg);

  bool (*get_setting_bool)(void* kodiBase, const char* id, bool* value);
  bool (*get_setting_int)(void* kodiBase, const char* id, int* value);
  bool (*get_setting_float)(void* kodiBase, const char* id, float* value);
  bool (*get_setting_string)(void* kodiBase, const char* id, char** value);

  bool (*set_setting_bool)(void* kodiBase, const char* id, bool value);
  bool (*set_setting_int)(void* kodiBase, const char* id, int value);
  bool (*set_setting_float)(void* kodiBase, const char* id, float value);
  bool (*set_setting_string)(void* kodiBase, const char* id, const char* value);

  AddonToKodiFuncTable_kodi* kodi;
  AddonToKodiFuncTable_kodi_audioengine* kodi_audioengine;
  AddonToKodiFuncTable_kodi_filesystem* kodi_filesystem;
  AddonToKodiFuncTable_kodi_gui* kodi_gui;
  AddonToKodiFuncTable_kodi_network *kodi_network;

  void* (*get_interface)(void* kodiBase, const char *name, const char *version);
} AddonToKodiFuncTable_Addon;

/*
 * Function tables from Kodi to addon
 */
typedef struct KodiToAddonFuncTable_Addon
{
  void (*destroy)();
  ADDON_STATUS (*get_status)();
  ADDON_STATUS (*create_instance)(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance, KODI_HANDLE parent);
  void (*destroy_instance)(int instanceType, KODI_HANDLE instance);
  ADDON_STATUS (*set_setting)(const char *settingName, const void *settingValue);
  ADDON_STATUS(*create_instance_ex)(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance, KODI_HANDLE parent, const char* version);
} KodiToAddonFuncTable_Addon;

/*
 * Main structure passed from kodi to addon with basic information needed to
 * create add-on.
 */
typedef struct AddonGlobalInterface
{
  // String with full path where add-on is installed (without his name on end)
  // Set from Kodi!
  const char* libBasePath;

  // Pointer of first created instance, used in case this add-on goes with single way
  // Set from Kodi!
  KODI_HANDLE firstKodiInstance;

  // Pointer to master base class inside add-on
  // Set from addon header!
  kodi::addon::CAddonBase* addonBase;

  // Pointer to a instance used on single way (together with this class)
  // Set from addon header!
  kodi::addon::IAddonInstance* globalSingleInstance;

  // Callback function tables from addon to Kodi
  // Set from Kodi!
  AddonToKodiFuncTable_Addon* toKodi;

  // Function tables from Kodi to addon
  // Set from addon header!
  KodiToAddonFuncTable_Addon* toAddon;
} AddonGlobalInterface;

} /* extern "C" */

//==============================================================================
namespace kodi {
namespace addon {
/*
 * Internal class to control various instance types with general parts defined
 * here.
 *
 * Mainly is this currently used to identify requested instance types.
 *
 * @note This class is not need to know during add-on development thats why
 * commented with "*".
 */
class IAddonInstance
{
public:
  explicit IAddonInstance(ADDON_TYPE type) : m_type(type) { }
  virtual ~IAddonInstance() = default;

  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
  {
    return ADDON_STATUS_NOT_IMPLEMENTED;
  }

  virtual ADDON_STATUS CreateInstanceEx(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance, const std::string &version)
  {
    return CreateInstance(instanceType, instanceID, instance, addonInstance);
  }

  const ADDON_TYPE m_type;
};
} /* namespace addon */
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
class CSettingValue
{
public:
  explicit CSettingValue(const void *settingValue) : m_settingValue(settingValue) {}

  bool empty() const { return (m_settingValue == nullptr) ? true : false; }
  std::string GetString() const { return (const char*)m_settingValue; }
  int GetInt() const { return *(const int*)m_settingValue; }
  unsigned int GetUInt() const { return *(const unsigned int*)m_settingValue; }
  bool GetBoolean() const { return *(const bool*)m_settingValue; }
  float GetFloat() const { return *(const float*)m_settingValue; }

private:
  const void *m_settingValue;
};
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
namespace addon {

/*
 * Internally used helper class to manage processing of a "C" structure in "CPP"
 * class.
 *
 * At constant, the "C" structure is copied, otherwise the given pointer is
 * superseded and is changeable.
 *
 * -----------------------------------------------------------------------------
 *
 * Example:
 *
 * ~~~~~~~~~~~~~{.cpp}
 * extern "C" typedef struct C_SAMPLE_DATA
 * {
 *   unsigned int iUniqueId;
 * } C_SAMPLE_DATA;
 *
 * class CPPSampleData : public CStructHdl<CPPSampleData, C_SAMPLE_DATA>
 * {
 * public:
 *   CPPSampleData() = default;
 *   CPPSampleData(const CPPSampleData& sample) : CStructHdl(sample) { }
 *   CPPSampleData(const C_SAMPLE_DATA* sample) : CStructHdl(sample) { }
 *   CPPSampleData(C_SAMPLE_DATA* sample) : CStructHdl(sample) { }
 *
 *   void SetUniqueId(unsigned int uniqueId) { m_cStructure->iUniqueId = uniqueId; }
 *   unsigned int GetUniqueId() const { return m_cStructure->iUniqueId; }
 * };
 *
 * ~~~~~~~~~~~~~
 *
 * It also works with the following example:
 *
 * ~~~~~~~~~~~~~{.cpp}
 * CPPSampleData test;
 * // Some work
 * C_SAMPLE_DATA* data = test;
 * // Give "data" to Kodi
 * ~~~~~~~~~~~~~
 */
template<class CPP_CLASS, typename C_STRUCT>
class CStructHdl
{
public:
  CStructHdl()
    : m_cStructure(new C_STRUCT)
    , m_owner(true)
  {
  }

  CStructHdl(const CPP_CLASS& cppClass)
    : m_cStructure(new C_STRUCT(*cppClass.m_cStructure))
    , m_owner(true)
  {
  }

  CStructHdl(const C_STRUCT* cStructure)
    : m_cStructure(new C_STRUCT({*cStructure}))
    , m_owner(true)
  {
  }

  CStructHdl(C_STRUCT* cStructure)
    : m_cStructure(cStructure)
  {
    assert(cStructure);
  }

  const CStructHdl& operator=(const CStructHdl& right)
  {
    assert(&right.m_cStructure);
    if (m_owner)
      delete m_cStructure;
    m_owner = true;
    m_cStructure = new C_STRUCT(*right.m_cStructure);
    return *this;
  }

  const CStructHdl& operator=(const C_STRUCT& right)
  {
    assert(&right);
    if (m_owner)
      delete m_cStructure;
    m_owner = true;
    m_cStructure = new C_STRUCT(*right);
    return *this;
  }

  virtual ~CStructHdl()
  {
    if (m_owner)
      delete m_cStructure;
  }

  operator C_STRUCT*() { return m_cStructure; }

protected:
  C_STRUCT* m_cStructure = nullptr;

private:
  bool m_owner = false;
};

/// Add-on main instance class.
class ATTRIBUTE_HIDDEN CAddonBase
{
public:
  CAddonBase()
  {
    CAddonBase::m_interface->toAddon->destroy = ADDONBASE_Destroy;
    CAddonBase::m_interface->toAddon->get_status = ADDONBASE_GetStatus;
    CAddonBase::m_interface->toAddon->create_instance = ADDONBASE_CreateInstance;
    CAddonBase::m_interface->toAddon->destroy_instance = ADDONBASE_DestroyInstance;
    CAddonBase::m_interface->toAddon->set_setting = ADDONBASE_SetSetting;
    // If version is present, we know that kodi has create_instance_ex implemented
    if (!CAddonBase::m_strGlobalApiVersion.empty())
      CAddonBase::m_interface->toAddon->create_instance_ex = ADDONBASE_CreateInstanceEx;
  }

  virtual ~CAddonBase() = default;

  virtual ADDON_STATUS Create() { return ADDON_STATUS_OK; }

  virtual ADDON_STATUS GetStatus() { return ADDON_STATUS_OK; }

  virtual ADDON_STATUS SetSetting(const std::string& settingName, const CSettingValue& settingValue) { return ADDON_STATUS_UNKNOWN; }

  //==========================================================================
  /// @ingroup cpp_kodi_addon_addonbase
  /// @brief Instance created
  ///
  /// @param[in] instanceType   The requested type of required instance, see \ref ADDON_TYPE.
  /// @param[in] instanceID     An individual identification key string given by Kodi.
  /// @param[in] instance       The instance handler used by Kodi must be passed
  ///                           to the classes created here. See in the example.
  /// @param[out] addonInstance The pointer to instance class created in addon.
  ///                           Needed to be able to identify them on calls.
  /// @return                   \ref ADDON_STATUS_OK if correct, for possible errors
  ///                           see \ref ADDON_STATUS
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here is a code example how this is used:**
  ///
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/AddonBase.h>
  ///
  /// ...
  ///
  /// /* If you use only one instance in your add-on, can be instanceType and
  ///  * instanceID ignored */
  /// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
  ///                                       std::string instanceID,
  ///                                       KODI_HANDLE instance,
  ///                                       KODI_HANDLE& addonInstance)
  /// {
  ///   if (instanceType == ADDON_INSTANCE_SCREENSAVER)
  ///   {
  ///     kodi::Log(ADDON_LOG_NOTICE, "Creating my Screensaver");
  ///     addonInstance = new CMyScreensaver(instance);
  ///     return ADDON_STATUS_OK;
  ///   }
  ///   else if (instanceType == ADDON_INSTANCE_VISUALIZATION)
  ///   {
  ///     kodi::Log(ADDON_LOG_NOTICE, "Creating my Visualization");
  ///     addonInstance = new CMyVisualization(instance);
  ///     return ADDON_STATUS_OK;
  ///   }
  ///   else if (...)
  ///   {
  ///     ...
  ///   }
  ///   return ADDON_STATUS_UNKNOWN;
  /// }
  ///
  /// ...
  ///
  /// ~~~~~~~~~~~~~
  ///
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
  {
    /* The handling below is intended for the case of the add-on only one
     * instance and this is integrated in the add-on base class.
     */

    /* Check about single instance usage */
    if (CAddonBase::m_interface->firstKodiInstance == instance && // the kodi side instance pointer must be equal to first one
        CAddonBase::m_interface->globalSingleInstance &&  // the addon side instance pointer must be set
        CAddonBase::m_interface->globalSingleInstance->m_type == instanceType) // and the requested type must be equal with used add-on class
    {
      addonInstance = CAddonBase::m_interface->globalSingleInstance;
      return ADDON_STATUS_OK;
    }

    return ADDON_STATUS_UNKNOWN;
  }
  //--------------------------------------------------------------------------

  virtual ADDON_STATUS CreateInstanceEx(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance, const std::string &version)
  {
    return CreateInstance(instanceType, instanceID, instance, addonInstance);
  }

  /* Background helper for GUI render systems, e.g. Screensaver or Visualization */
  std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper;

  /* Global variables of class */
  static AddonGlobalInterface* m_interface; // Interface function table to hold addresses on add-on and from kodi
  static std::string m_strGlobalApiVersion;

/*private:*/ /* Needed public as long the old call functions becomes used! */
  static inline void ADDONBASE_Destroy()
  {
    delete CAddonBase::m_interface->addonBase;
    CAddonBase::m_interface->addonBase = nullptr;
  }

  static inline ADDON_STATUS ADDONBASE_GetStatus() { return CAddonBase::m_interface->addonBase->GetStatus(); }

  static inline ADDON_STATUS ADDONBASE_SetSetting(const char *settingName, const void *settingValue)
  {
    return CAddonBase::m_interface->addonBase->SetSetting(settingName, CSettingValue(settingValue));
  }

  static inline ADDON_STATUS ADDONBASE_CreateInstance(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance, KODI_HANDLE parent)
  {
    return ADDONBASE_CreateInstanceEx(instanceType, instanceID, instance, addonInstance, parent, "");
  }

  static inline ADDON_STATUS ADDONBASE_CreateInstanceEx(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance, KODI_HANDLE parent, const char* version)
  {
    ADDON_STATUS status = ADDON_STATUS_NOT_IMPLEMENTED;
    if (parent != nullptr)
      status = static_cast<IAddonInstance*>(parent)->CreateInstanceEx(instanceType, instanceID, instance, *addonInstance, version);
    if (status == ADDON_STATUS_NOT_IMPLEMENTED)
      status = CAddonBase::m_interface->addonBase->CreateInstanceEx(instanceType, instanceID, instance, *addonInstance, version);
    if (*addonInstance == nullptr)
      throw std::logic_error("kodi::addon::CAddonBase CreateInstanceEx returns a empty instance pointer!");

    if (static_cast<::kodi::addon::IAddonInstance*>(*addonInstance)->m_type != instanceType)
      throw std::logic_error("kodi::addon::CAddonBase CreateInstanceEx with difference on given and returned instance type!");

    return status;
  }

  static inline void ADDONBASE_DestroyInstance(int instanceType, KODI_HANDLE instance)
  {
    if (CAddonBase::m_interface->globalSingleInstance == nullptr && instance != CAddonBase::m_interface->addonBase)
    {
      if (static_cast<::kodi::addon::IAddonInstance*>(instance)->m_type == instanceType)
        delete static_cast<::kodi::addon::IAddonInstance*>(instance);
      else
        throw std::logic_error("kodi::addon::CAddonBase DestroyInstance called with difference on given and present instance type!");
    }
  }
};
} /* namespace addon */
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
inline std::string GetAddonPath(const std::string& append = "")
{
  char* str = ::kodi::addon::CAddonBase::m_interface->toKodi->get_addon_path(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase);
  std::string ret = str;
  ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, str);
  if (!append.empty())
  {
    if (append.at(0) != '\\' &&
        append.at(0) != '/')
#ifdef TARGET_WINDOWS
      ret.append("\\");
#else
      ret.append("/");
#endif
    ret.append(append);
  }
  return ret;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
inline std::string GetBaseUserPath(const std::string& append = "")
{
  char* str = ::kodi::addon::CAddonBase::m_interface->toKodi->get_base_user_path(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase);
  std::string ret = str;
  ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, str);
  if (!append.empty())
  {
    if (append.at(0) != '\\' &&
        append.at(0) != '/')
#ifdef TARGET_WINDOWS
      ret.append("\\");
#else
      ret.append("/");
#endif
    ret.append(append);
  }
  return ret;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
inline std::string GetLibPath()
{
  return ::kodi::addon::CAddonBase::m_interface->libBasePath;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
inline void Log(const AddonLog loglevel, const char* format, ...)
{
  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ::kodi::addon::CAddonBase::m_interface->toKodi->addon_log_msg(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, loglevel, buffer);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline bool CheckSettingString(const std::string& settingName, std::string& settingValue)
{
  char* buffer = nullptr;
  bool ret = ::kodi::addon::CAddonBase::m_interface->toKodi->get_setting_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), &buffer);
  if (buffer)
  {
    if (ret)
      settingValue = buffer;
    ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, buffer);
  }
  return ret;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline std::string GetSettingString(const std::string& settingName)
{
  std::string settingValue;
  CheckSettingString(settingName, settingValue);
  return settingValue;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline void SetSettingString(const std::string& settingName, const std::string& settingValue)
{
  ::kodi::addon::CAddonBase::m_interface->toKodi->set_setting_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), settingValue.c_str());
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline bool CheckSettingInt(const std::string& settingName, int& settingValue)
{
  return ::kodi::addon::CAddonBase::m_interface->toKodi->get_setting_int(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), &settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline int GetSettingInt(const std::string& settingName)
{
  int settingValue = 0;
  CheckSettingInt(settingName, settingValue);
  return settingValue;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline void SetSettingInt(const std::string& settingName, int settingValue)
{
  ::kodi::addon::CAddonBase::m_interface->toKodi->set_setting_int(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline bool CheckSettingBoolean(const std::string& settingName, bool& settingValue)
{
  return ::kodi::addon::CAddonBase::m_interface->toKodi->get_setting_bool(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), &settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline bool GetSettingBoolean(const std::string& settingName)
{
  bool settingValue = false;
  CheckSettingBoolean(settingName, settingValue);
  return settingValue;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline void SetSettingBoolean(const std::string& settingName, bool settingValue)
{
  ::kodi::addon::CAddonBase::m_interface->toKodi->set_setting_bool(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline bool CheckSettingFloat(const std::string& settingName, float& settingValue)
{
  return ::kodi::addon::CAddonBase::m_interface->toKodi->get_setting_float(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), &settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline float GetSettingFloat(const std::string& settingName)
{
  float settingValue = 0.0f;
  CheckSettingFloat(settingName, settingValue);
  return settingValue;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline void SetSettingFloat(const std::string& settingName, float settingValue)
{
  ::kodi::addon::CAddonBase::m_interface->toKodi->set_setting_float(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, settingName.c_str(), settingValue);
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
inline std::string TranslateAddonStatus(ADDON_STATUS status)
{
  switch (status)
  {
    case ADDON_STATUS_OK:
      return "OK";
    case ADDON_STATUS_LOST_CONNECTION:
      return "Lost Connection";
    case ADDON_STATUS_NEED_RESTART:
      return "Need Restart";
    case ADDON_STATUS_NEED_SETTINGS:
      return "Need Settings";
    case ADDON_STATUS_UNKNOWN:
      return "Unknown error";
    case ADDON_STATUS_PERMANENT_FAILURE:
      return "Permanent failure";
    case ADDON_STATUS_NOT_IMPLEMENTED:
      return "Not implemented";
    default:
      break;
  }
  return "Unknown";
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns a function table to a named interface
///
/// @return pointer to struct containing interface functions
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// #include <kodi/platform/foo.h>
/// ...
/// FuncTable_foo *table = kodi::GetPlatformInfo(foo_name, foo_version);
/// ...
/// ~~~~~~~~~~~~~
///
inline void* GetInterface(const std::string &name, const std::string &version)
{
  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;

  return toKodi->get_interface(toKodi->kodiBase, name.c_str(), version.c_str());
}
} /* namespace kodi */

/*! addon creation macro
 * @todo cleanup this stupid big macro
 * This macro includes now all for add-on's needed functions. This becomes a bigger
 * rework after everything is done on Kodi itself, currently is this way needed
 * to have compatibility with not reworked interfaces.
 *
 * Becomes really cleaned up soon :D
 */
#define ADDONCREATOR(AddonClass) \
  extern "C" __declspec(dllexport) void get_addon(void* pAddon) {} \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_Create(KODI_HANDLE addonInterface, void *unused) \
  { \
    kodi::addon::CAddonBase::m_interface = static_cast<AddonGlobalInterface*>(addonInterface); \
    kodi::addon::CAddonBase::m_interface->addonBase = new AddonClass; \
    return kodi::addon::CAddonBase::m_interface->addonBase->Create(); \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_CreateEx(KODI_HANDLE addonInterface, const char* globalApiVersion, void *unused) \
  { \
    kodi::addon::CAddonBase::m_strGlobalApiVersion = globalApiVersion; \
    return ADDON_Create(addonInterface, unused); \
  } \
  extern "C" __declspec(dllexport) void ADDON_Destroy() \
  { \
    kodi::addon::CAddonBase::ADDONBASE_Destroy(); \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_GetStatus() \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_GetStatus(); \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue) \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_SetSetting(settingName, settingValue); \
  } \
  extern "C" __declspec(dllexport) const char* ADDON_GetTypeVersion(int type) \
  { \
    return kodi::addon::GetTypeVersion(type); \
  } \
  extern "C" __declspec(dllexport) const char* ADDON_GetTypeMinVersion(int type) \
  { \
    return kodi::addon::GetTypeMinVersion(type); \
  } \
  AddonGlobalInterface* kodi::addon::CAddonBase::m_interface = nullptr; \
  std::string kodi::addon::CAddonBase::m_strGlobalApiVersion;

