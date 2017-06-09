#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */
#include <cstdlib>
#include <cstring>
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
  #if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
    #define ATTRIBUTE_PACKED __attribute__ ((packed))
    #define PRAGMA_PACK 0
  #endif
#endif

#if !defined(ATTRIBUTE_PACKED)
  #define ATTRIBUTE_PACKED
  #define PRAGMA_PACK 1
#endif

#include "versions.h"

namespace kodi { namespace addon { class CAddonBase; }}
namespace kodi { namespace addon { class IAddonInstance; }}

extern "C" {

//==============================================================================
/// Standard undefined pointer handle
typedef void* KODI_HANDLE;
//------------------------------------------------------------------------------

//==============================================================================
///
typedef enum ADDON_STATUS
{
  ///
  ADDON_STATUS_OK,

  ///
  ADDON_STATUS_LOST_CONNECTION,

  ///
  ADDON_STATUS_NEED_RESTART,

  ///
  ADDON_STATUS_NEED_SETTINGS,

  ///
  ADDON_STATUS_UNKNOWN,

  ///
  ADDON_STATUS_NEED_SAVEDSETTINGS,

  /// permanent failure, like failing to resolve methods
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
  IAddonInstance(ADDON_TYPE type) : m_type(type) { }
  virtual ~IAddonInstance() = default;

  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
  {
    return ADDON_STATUS_NOT_IMPLEMENTED;
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
  CSettingValue(const void *settingValue) : m_settingValue(settingValue) {}

  bool empty() const { return (m_settingValue == nullptr) ? true : false; }
  std::string GetString() const { return (char*)m_settingValue; }
  int GetInt() const { return *(int*)m_settingValue; }
  unsigned int GetUInt() const { return *(unsigned int*)m_settingValue; }
  bool GetBoolean() const { return *(bool*)m_settingValue; }
  float GetFloat() const { return *(float*)m_settingValue; }

private:
  const void *m_settingValue;
};
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
namespace addon {
/// Add-on main instance class.
class CAddonBase
{
public:
  CAddonBase()
  {
    CAddonBase::m_interface->toAddon->destroy = ADDONBASE_Destroy;
    CAddonBase::m_interface->toAddon->get_status = ADDONBASE_GetStatus;
    CAddonBase::m_interface->toAddon->create_instance = ADDONBASE_CreateInstance;
    CAddonBase::m_interface->toAddon->destroy_instance = ADDONBASE_DestroyInstance;
    CAddonBase::m_interface->toAddon->set_setting = ADDONBASE_SetSetting;
  }

  virtual ~CAddonBase() = default;

  virtual ADDON_STATUS Create() { return ADDON_STATUS_OK; }

  virtual ADDON_STATUS GetStatus() { return ADDON_STATUS_OK; }

  virtual ADDON_STATUS SetSetting(const std::string& settingName, const CSettingValue& settingValue) { return ADDON_STATUS_UNKNOWN; }

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

  /* Global variables of class */
  static AddonGlobalInterface* m_interface; // Interface function table to hold addresses on add-on and from kodi

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
    ADDON_STATUS status = ADDON_STATUS_NOT_IMPLEMENTED;
    if (parent != nullptr)
      status = static_cast<IAddonInstance*>(parent)->CreateInstance(instanceType, instanceID, instance, *addonInstance);
    if (status == ADDON_STATUS_NOT_IMPLEMENTED)
      status = CAddonBase::m_interface->addonBase->CreateInstance(instanceType, instanceID, instance, *addonInstance);
    if (*addonInstance == nullptr)
      throw std::logic_error("kodi::addon::CAddonBase CreateInstance returns a empty instance pointer!");

    if (static_cast<::kodi::addon::IAddonInstance*>(*addonInstance)->m_type != instanceType)
      throw std::logic_error("kodi::addon::CAddonBase CreateInstance with difference on given and returned instance type!");

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
    case ADDON_STATUS_NEED_SAVEDSETTINGS:
      return "Need saved settings";
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
  AddonGlobalInterface* kodi::addon::CAddonBase::m_interface = nullptr;
