/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon_base.h"
#include "addons/kodi-dev-kit/include/kodi/versions.h"
#include "threads/CriticalSection.h"

#include <memory>
#include <string>

class CSetting;

namespace ADDON
{

class CAddonDll;
using AddonDllPtr = std::shared_ptr<CAddonDll>;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class CBinaryAddonBase;
using BinaryAddonBasePtr = std::shared_ptr<CBinaryAddonBase>;

class IAddonInstanceHandler
{
public:
  /**
   * @brief Class constructor for handling add-on instance processes, allowing
   * an add-on to handle multiple work simultaneously and independently.
   *
   * @param[in] type The associated add-on type which is processed in the running
   *                 instance.
   * @param[in] addonInfo Class for querying available add-on information (e.g.
   *                      content declared in addon.xml).
   * @param[in] parentInstance *[opt]* Above running add-on instance which starts this
   *                           instance. Used to have the associated class for
   *                           work to open in the add-on.\n\n
   *                           **Currently used values:**
   * | Parent | Target | Description
   * |--------|--------|-------------
   * | @ref kodi::addon::CInstanceInputStream | @ref kodi::addon::CInstanceVideoCodec | In order to be able to access the overlying input stream instance in the video codec created by Kodi on the add-on.
   * @param[in] uniqueWorkID *[opt]* Identification value intended to pass any special
   *                         values to the instance to be opened.
   *                         If not used, the IAddonInstanceHandler class pointer
   *                         is used as a string.\n\n
   *                         **Currently used values:**
   * | Add-on instance type | Description
   * |----------------------|---------------------
   * | @ref kodi::addon::CInstanceInputStream "Inputstream" | To transfer special values to inputstream using the property @ref STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID from external space, for example PVR add-on which also supports inputstream can exchange special values with it, e.g. select the necessary add-on processing class, since it is not known at the start what is being executed ( live TV, radio, recordings...) and add-on may use different classes.
   * | All other | The used class pointer of Kodi's @ref IAddonInstanceHandler is used as a value to have an individually different value.
   */
  IAddonInstanceHandler(ADDON_TYPE type,
                        const AddonInfoPtr& addonInfo,
                        AddonInstanceId instanceId = ADDON_INSTANCE_ID_UNUSED,
                        KODI_HANDLE parentInstance = nullptr,
                        const std::string& uniqueWorkID = "");
  virtual ~IAddonInstanceHandler();

  ADDON_TYPE UsedType() const { return m_type; }
  AddonInstanceId InstanceId() const { return m_instanceId; }
  const std::string& UniqueWorkID() { return m_uniqueWorkID; }

  std::string ID() const;
  AddonInstanceId InstanceID() const;
  std::string Name() const;
  std::string Author() const;
  std::string Icon() const;
  std::string Path() const;
  std::string Profile() const;
  CAddonVersion Version() const;

  ADDON_STATUS CreateInstance();
  void DestroyInstance();
  const AddonDllPtr& Addon() const { return m_addon; }
  AddonInfoPtr GetAddonInfo() const { return m_addonInfo; }

  virtual void OnPreInstall() {}
  virtual void OnPostInstall(bool update, bool modal) {}
  virtual void OnPreUnInstall() {}
  virtual void OnPostUnInstall() {}

protected:
  KODI_ADDON_INSTANCE_INFO m_info{};
  KODI_ADDON_INSTANCE_STRUCT m_ifc{};

private:
  std::shared_ptr<CSetting> GetSetting(const std::string& setting);

  static char* get_instance_user_path(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl);
  static bool is_instance_setting_using_default(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                                const char* id);
  static bool get_instance_setting_bool(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                        const char* id,
                                        bool* value);
  static bool get_instance_setting_int(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                       const char* id,
                                       int* value);
  static bool get_instance_setting_float(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                         const char* id,
                                         float* value);
  static bool get_instance_setting_string(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                          const char* id,
                                          char** value);
  static bool set_instance_setting_bool(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                        const char* id,
                                        bool value);
  static bool set_instance_setting_int(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                       const char* id,
                                       int value);
  static bool set_instance_setting_float(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                         const char* id,
                                         float value);
  static bool set_instance_setting_string(const KODI_ADDON_INSTANCE_BACKEND_HDL hdl,
                                          const char* id,
                                          const char* value);

  const ADDON_TYPE m_type;
  const AddonInstanceId m_instanceId;
  std::string m_uniqueWorkID;
  KODI_HANDLE m_parentInstance;
  AddonInfoPtr m_addonInfo;
  BinaryAddonBasePtr m_addonBase;
  AddonDllPtr m_addon;
  static CCriticalSection m_cdSec;
};

} /* namespace ADDON */
