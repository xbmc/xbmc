#pragma once
/*
 *      Copyright (C) 2005-2016 Team Kodi
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
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include "versions.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Standard undefined pointer handle */
typedef void* KODI_HANDLE;

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

/*
 *
 */
typedef struct
{
  int           type;
  char*         id;
  char*         label;
  int           current;
  char**        entry;
  unsigned int  entry_elements;
} ADDON_StructSetting;

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

/* To have a on add-on and kodi itself handled string always on known size! */
#define ADDON_STANDARD_STRING_LENGTH 1024
#define ADDON_STANDARD_STRING_LENGTH_SMALL 256

#ifdef __cplusplus
namespace kodi {
namespace addon {

  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  //
  class IAddonInstance
  {
  public:
    IAddonInstance(ADDON_TYPE type) : m_type(type) { }
    virtual ~IAddonInstance() { }

    const ADDON_TYPE m_type;
  };
  //
  //=-----=------=------=------=------=------=------=------=------=------=-----=


  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Add-on settings handle class
  //
  class CAddonSetting
  {
  public:
    enum SETTING_TYPE { NONE=0, CHECK, SPIN };

    CAddonSetting(SETTING_TYPE type, std::string id, std::string label)
      : Type(type),
        Id(id),
        Label(label),
        Current(0)
    {
    }

    CAddonSetting(const CAddonSetting &rhs) // copy constructor
    {
      Id = rhs.Id;
      Label = rhs.Label;
      Current = rhs.Current;
      Type = rhs.Type;
      for (unsigned int i = 0; i < rhs.Entries.size(); ++i)
        Entries.push_back(rhs.Entries[i]);
    }

    void AddEntry(std::string label)
    {
      if (Label.empty() || Type != SPIN)
        return;
      Entries.push_back(Label);
    }

    // data members
    SETTING_TYPE Type;
    std::string Id;
    std::string Label;
    int Current;
    std::vector<std::string> Entries;
  };
  //
  //=-----=------=------=------=------=------=------=------=------=------=-----=


  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Function used on Kodi itself to transfer back from add-on given data with
  // "ADDON_StructSetting***" to "std::vector<CAddonSetting>"
  //
  // Note: Not needed on add-on itself, only here to have all related parts on
  // same place!
  //
  static inline void StructToVec(unsigned int elements, ADDON_StructSetting*** sSet, std::vector<CAddonSetting> *vecSet)
  {
    if (elements == 0)
      return;

    vecSet->clear();
    for(unsigned int i = 0; i < elements; i++)
    {
      CAddonSetting vSet((CAddonSetting::SETTING_TYPE)(*sSet)[i]->type, (*sSet)[i]->id, (*sSet)[i]->label);
      if((*sSet)[i]->type == CAddonSetting::SPIN)
      {
        for(unsigned int j=0;j<(*sSet)[i]->entry_elements;j++)
        {
          vSet.AddEntry((*sSet)[i]->entry[j]);
        }
      }
      vSet.Current = (*sSet)[i]->current;
      vecSet->push_back(vSet);
    }
  }
  //
  //=-----=------=------=------=------=------=------=------=------=------=-----=


  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
  // Add-on main instance class.
  //
  class CAddonBase
  {
  public:
    CAddonBase()
    {
    }

    virtual ADDON_STATUS Create() { return ADDON_STATUS_OK; }

    virtual ADDON_STATUS GetStatus() { return ADDON_STATUS_OK; }

    virtual bool HasSettings() { return false; }

    virtual bool GetSettings(std::vector<CAddonSetting>& settings) { return false; }

    virtual ADDON_STATUS SetSetting(std::string& settingName, const void *settingValue) { return ADDON_STATUS_UNKNOWN; }

    virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
    {
      /* The handling below is intended for the case of the add-on only one
       * instance and this is integrated in the add-on base class.
       */

      /* Check about single instance usage */
      if (m_firstKodiInstance == instance && // the kodi side instance pointer must be equal to first one
          m_globalSingleInstance &&  // the addon side instance pointer must be set
          static_cast<::kodi::addon::IAddonInstance*>(m_globalSingleInstance)->m_type == instanceType) // and the requested type must be equal with used add-on class
      {
        addonInstance = m_globalSingleInstance;
        return ADDON_STATUS_OK;
      }

      return ADDON_STATUS_UNKNOWN;
    }

    /* Global variables of class */
    static CAddonBase* m_createdAddon; // Base add-on class pointer, needed to save startet main class
    static KODI_HANDLE m_globalSingleInstance; // Pointer to a instance used on single way (together with this class)
    static KODI_HANDLE m_firstKodiInstance; // Pointer of first created instance, used in case this add-on goes with single way

  /*private:*/ /* Needed public as long the old call functions becomes used! */
    static inline void ADDONBASE_Destroy()
    {
      delete CAddonBase::m_createdAddon;
      CAddonBase::m_createdAddon = nullptr;
    }

    static inline ADDON_STATUS ADDONBASE_GetStatus() { return CAddonBase::m_createdAddon->GetStatus(); }

    static inline bool ADDONBASE_HasSettings() { return CAddonBase::m_createdAddon->HasSettings(); }

    static inline unsigned int ADDONBASE_GetSettings(ADDON_StructSetting ***sSet)
    {
      std::vector<CAddonSetting> settings;
      if (CAddonBase::m_createdAddon->GetSettings(settings))
      {
        *sSet = nullptr;
        if (settings.empty())
          return 0;

        *sSet = (ADDON_StructSetting**)malloc(settings.size()*sizeof(ADDON_StructSetting*));
        for (unsigned int i = 0;i < settings.size(); ++i)
        {
          (*sSet)[i] = nullptr;
          (*sSet)[i] = (ADDON_StructSetting*)malloc(sizeof(ADDON_StructSetting));
          (*sSet)[i]->id = nullptr;
          (*sSet)[i]->label = nullptr;

          if (!settings[i].Id.empty() && !settings[i].Label.empty())
          {
            (*sSet)[i]->id = strdup(settings[i].Id.c_str());
            (*sSet)[i]->label = strdup(settings[i].Label.c_str());
            (*sSet)[i]->type = settings[i].Type;
            (*sSet)[i]->current = settings[i].Current;
            (*sSet)[i]->entry_elements = 0;
            (*sSet)[i]->entry = nullptr;
            if (settings[i].Type == CAddonSetting::SPIN && !settings[i].Entries.empty())
            {
              (*sSet)[i]->entry = (char**)malloc(settings[i].Entries.size()*sizeof(char**));
              for (unsigned int j = 0; j < settings[i].Entries.size(); ++j)
              {
                if (!settings[i].Entries[j].empty())
                {
                  (*sSet)[i]->entry[j] = strdup(settings[i].Entries[j].c_str());
                  (*sSet)[i]->entry_elements++;
                }
              }
            }
          }
        }

        return settings.size();
      }
      return 0;
    }

    static inline ADDON_STATUS ADDONBASE_SetSetting(const char *settingName, const void *settingValue)
    {
      std::string name = settingName;
      ADDON_STATUS ret = CAddonBase::m_createdAddon->SetSetting(name, settingValue);
      std::strcpy((char*)settingName, name.c_str());
      return ret;
    }

    static inline void ADDONBASE_FreeSettings(unsigned int elements, ADDON_StructSetting*** set)
    {
      if (elements == 0)
        return;

      for (unsigned int i = 0; i < elements; ++i)
      {
        if ((*set)[i]->type == CAddonSetting::SPIN)
        {
          for (unsigned int j = 0; j < (*set)[i]->entry_elements; ++j)
          {
            free((*set)[i]->entry[j]);
          }
          free((*set)[i]->entry);
        }
        free((*set)[i]->id);
        free((*set)[i]->label);
        free((*set)[i]);
      }
      free(*set);
    }

    static inline ADDON_STATUS ADDONBASE_CreateInstance(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance)
    {
      ADDON_STATUS status = CAddonBase::m_createdAddon->CreateInstance(instanceType, instanceID, instance, *addonInstance);
      if (*addonInstance == nullptr)
        throw std::logic_error("kodi::addon::CAddonBase CreateInstance returns a empty instance pointer!");

      if (static_cast<::kodi::addon::IAddonInstance*>(*addonInstance)->m_type != instanceType)
        throw std::logic_error("kodi::addon::CAddonBase CreateInstance with difference on given and returned instance type!");

      return status;
    }

    static inline void ADDONBASE_DestroyInstance(int instanceType, KODI_HANDLE instance)
    {
      if (m_globalSingleInstance == nullptr && instance != CAddonBase::m_createdAddon)
      {
        if (static_cast<::kodi::addon::IAddonInstance*>(instance)->m_type == instanceType)
          delete static_cast<::kodi::addon::IAddonInstance*>(instance);
        else
          throw std::logic_error("kodi::addon::CAddonBase DestroyInstance called with difference on given and present instance type!");
      }
    }
  };
  //
  //=-----=------=------=------=------=------=------=------=------=------=-----=

} /* namespace addon */
} /* namespace kodi */
} /* extern "C" */
#endif /* __cplusplus */


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
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_Create(void *cb, void *firstKodiInstance) \
  { \
    kodi::addon::CAddonBase::m_firstKodiInstance = firstKodiInstance; \
    kodi::addon::CAddonBase::m_createdAddon = new AddonClass; \
    return kodi::addon::CAddonBase::m_createdAddon->Create(); \
  } \
  extern "C" __declspec(dllexport) void ADDON_Destroy() \
  { \
    kodi::addon::CAddonBase::ADDONBASE_Destroy(); \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_GetStatus() \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_GetStatus(); \
  } \
  extern "C" __declspec(dllexport) bool ADDON_HasSettings() \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_HasSettings(); \
  } \
  extern "C" __declspec(dllexport) unsigned int ADDON_GetSettings(ADDON_StructSetting*** sSet) \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_GetSettings(sSet); \
  } \
  extern "C" __declspec(dllexport) void ADDON_FreeSettings() \
  { \
    kodi::addon::CAddonBase::ADDONBASE_FreeSettings(0, nullptr); /* Currently bad but becomes soon changed! */ \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue) \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_SetSetting(settingName, settingValue); \
  } \
  extern "C" __declspec(dllexport) const char* ADDON_GetTypeVersion(int type) \
  { \
    return kodi::addon::GetTypeVersion(type); \
  } \
  extern "C" __declspec(dllexport) ADDON_STATUS ADDON_CreateInstance(int instanceType, const char* instanceID, KODI_HANDLE instance, KODI_HANDLE* addonInstance) \
  { \
    return kodi::addon::CAddonBase::ADDONBASE_CreateInstance(instanceType, instanceID, instance, addonInstance); \
  } \
  extern "C" __declspec(dllexport) void ADDON_DestroyInstance(int instanceType, KODI_HANDLE instance) \
  { \
    kodi::addon::CAddonBase::ADDONBASE_DestroyInstance(instanceType, instance); \
  } \
  kodi::addon::CAddonBase* kodi::addon::CAddonBase::m_createdAddon = nullptr; \
  KODI_HANDLE kodi::addon::CAddonBase::m_globalSingleInstance = nullptr; \
  KODI_HANDLE kodi::addon::CAddonBase::m_firstKodiInstance = nullptr;
