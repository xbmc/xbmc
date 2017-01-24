#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/AddonProperties.h"
#include "utils/XBMCTinyXML.h"
#include "guilib/LocalizeStrings.h"
#include "utils/ISerializable.h"

#include <stdint.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>


class TiXmlElement;
class CAddonCallbacksAddon;
class CVariant;

typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_extension_t cp_extension_t;

namespace ADDON
{
  class CAddon;
  typedef std::shared_ptr<CAddon> AddonPtr;
  class CSkinInfo;
  typedef std::shared_ptr<CSkinInfo> SkinPtr;
  class CPluginSource;
  typedef std::shared_ptr<CPluginSource> PluginPtr;

  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;

  const char* const ORIGIN_SYSTEM = "b6a50484-93a0-4afb-a01c-8d17e059feda";

void OnEnabled(const std::string& id);
void OnDisabled(const std::string& id);
void OnPreInstall(const AddonPtr& addon);
void OnPostInstall(const AddonPtr& addon, bool update, bool modal);
void OnPreUnInstall(const AddonPtr& addon);
void OnPostUnInstall(const AddonPtr& addon);

class CAddon : public std::enable_shared_from_this<CAddon>
{
public:
  explicit CAddon(AddonProps props);
  virtual ~CAddon() {}

  virtual TYPE Type() const { return m_props.m_type; }
  virtual TYPE FullType() const { return Type(); }
  virtual bool IsType(TYPE type) const { return type == m_props.m_type; }
  virtual std::string ID() const { return m_props.m_id; }
  virtual std::string Name() const { return m_props.m_name; }
  virtual bool IsInUse() const { return false; };
  virtual AddonVersion Version() const { return m_props.m_version; }
  virtual AddonVersion MinVersion() const { return m_props.m_minversion; }
  virtual std::string Summary() const { return m_props.m_summary; }
  virtual std::string Description() const { return m_props.m_description; }
  virtual std::string Path() const { return m_props.m_path; }
  virtual std::string Profile() const { return m_profilePath; }
  virtual std::string LibPath() const;
  virtual std::string Author() const { return m_props.m_author; }
  virtual std::string ChangeLog() const { return m_props.m_changelog; }
  virtual std::string FanArt() const { return m_props.m_fanart; }
  virtual std::string Icon() const { return m_props.m_icon; };
  virtual std::vector<std::string> Screenshots() const { return m_props.m_screenshots; };
  virtual std::string Disclaimer() const { return m_props.m_disclaimer; }
  virtual std::string Broken() const { return m_props.m_broken; }
  virtual CDateTime InstallDate() const { return m_props.m_installDate; }
  virtual CDateTime LastUpdated() const { return m_props.m_lastUpdated; }
  virtual CDateTime LastUsed() const { return m_props.m_lastUsed; }
  virtual std::string Origin() const { return m_props.m_origin; }
  virtual uint64_t PackageSize() const { return m_props.m_packageSize; }
  virtual const InfoMap& ExtraInfo() const { return m_props.m_extrainfo; }
  virtual const ADDONDEPS& GetDeps() const { return m_props.m_dependencies; }

  /*! \brief Check whether the this addon can be configured or not
   \return true if the addon has settings, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool HasSettings();

  /*! \brief Check whether the user has configured this addon or not
   \return true if previously saved settings are found, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, GetSetting, UpdateSetting
   */
  virtual bool HasUserSettings();

  /*! \brief Save any user configured settings
   \sa LoadSettings, LoadUserSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual void SaveSettings();

  /*! \brief Update a user-configured setting with a new value
   \param key the id of the setting to update
   \param value the value that the setting should take
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  virtual void UpdateSetting(const std::string& key, const std::string& value);

  virtual void UpdateSettings(std::map<std::string, std::string>& settings);

  /*! \brief Retrieve a particular settings value
   If a previously configured user setting is available, we return it's value, else we return the default (if available)
   \param key the id of the setting to retrieve
   \return the current value of the setting, or the default if the setting has yet to be configured.
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  virtual std::string GetSetting(const std::string& key);

  virtual TiXmlElement* GetSettingsXML();

  /*! \brief get the required version of a dependency.
   \param dependencyID the addon ID of the dependency.
   \return the version this addon requires.
   */
  virtual AddonVersion GetDependencyVersion(const std::string &dependencyID) const;

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  virtual bool MeetsVersion(const AddonVersion &version) const;

  /*! \brief Load the default settings and override these with any previously configured user settings
   \param bForce force the load of settings even if they are already loaded (reload)
   \return true if settings exist, false otherwise
   \sa LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool LoadSettings(bool bForce = false);

  virtual bool ReloadSettings();

  /*! \brief callback for when this add-on is disabled.
   Use to perform any needed actions (e.g. stop a service)
   */
  virtual void OnDisabled() {};

  /*! \brief callback for when this add-on is enabled.
   Use to perform any needed actions (e.g. start a service)
   */
  virtual void OnEnabled() {};

  /*! \brief retrieve the running instance of an add-on if it persists while running.
   */
  virtual AddonPtr GetRunningInstance() const { return AddonPtr(); }

  virtual void OnPreInstall() {};
  virtual void OnPostInstall(bool update, bool modal) {};
  virtual void OnPreUnInstall() {};
  virtual void OnPostUnInstall() {};

protected:
  /*! \brief Load the user settings
   \return true if user settings exist, false otherwise
   \sa LoadSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool LoadUserSettings();

  /* \brief Whether there are settings to be saved
   \sa SaveSettings
   */
  virtual bool HasSettingsToSave() const;

  /*! \brief Parse settings from an XML document
   \param doc XML document to parse for settings
   \param loadDefaults if true, the default attribute is used and settings are reset prior to parsing, else the value attribute is used.
   \return true if settings are loaded, false otherwise
   \sa SettingsToXML
   */
  virtual bool SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults = false);

  /*! \brief Write settings into an XML document
   \param doc XML document to receive the settings
   \sa SettingsFromXML
   */
  virtual void SettingsToXML(CXBMCTinyXML &doc) const;

  const AddonProps m_props;
  CXBMCTinyXML      m_addonXmlDoc;
  bool              m_settingsLoaded;
  bool              m_userSettingsLoaded;

private:
  bool m_hasSettings;

  std::string m_profilePath;
  std::string m_userSettingsPath;
  std::map<std::string, std::string> m_settings;
};

}; /* namespace ADDON */

