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

#include "IAddon.h"
#include "addons/AddonVersion.h"
#include "utils/XBMCTinyXML.h"
#include "guilib/LocalizeStrings.h"
#include "utils/ISerializable.h"
#include <vector>

class TiXmlElement;
class CAddonCallbacksAddon;
class CVariant;

typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_extension_t cp_extension_t;

namespace ADDON
{
  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;

  const char* const ORIGIN_SYSTEM = "b6a50484-93a0-4afb-a01c-8d17e059feda";

// utils
std::string TranslateType(TYPE type, bool pretty=false);
std::string GetIcon(TYPE type);
TYPE TranslateType(const std::string &string);

void OnEnabled(const std::string& id);
void OnDisabled(const std::string& id);
void OnPreInstall(const AddonPtr& addon);
void OnPostInstall(const AddonPtr& addon, bool update, bool modal);
void OnPreUnInstall(const AddonPtr& addon);
void OnPostUnInstall(const AddonPtr& addon);


class AddonProps
{
public:
  AddonProps() : type(ADDON_UNKNOWN), packageSize(0) {};
  AddonProps(std::string id, TYPE type) : id(std::move(id)), type(type), packageSize(0) {}

  std::string id;
  TYPE type;
  AddonVersion version{"0.0.0"};
  AddonVersion minversion{"0.0.0"};
  std::string name;
  std::string license;
  std::string summary;
  std::string description;
  std::string libname;
  std::string author;
  std::string source;
  std::string path;
  std::string icon;
  std::string changelog;
  std::string fanart;
  std::vector<std::string> screenshots;
  std::string disclaimer;
  ADDONDEPS dependencies;
  std::string broken;
  InfoMap extrainfo;
  CDateTime installDate;
  CDateTime lastUpdated;
  CDateTime lastUsed;
  std::string origin;
  uint64_t packageSize;
};


class CAddon : public IAddon
{
public:
  explicit CAddon(AddonProps props);
  virtual ~CAddon() {}

  TYPE Type() const override { return m_props.type; }
  TYPE FullType() const override { return Type(); }
  bool IsType(TYPE type) const override { return type == m_props.type; }
  std::string ID() const override{ return m_props.id; }
  std::string Name() const override { return m_props.name; }
  bool IsInUse() const override{ return false; };
  AddonVersion Version() const override { return m_props.version; }
  AddonVersion MinVersion() const override { return m_props.minversion; }
  std::string Summary() const override { return m_props.summary; }
  std::string Description() const override { return m_props.description; }
  std::string Path() const override { return m_props.path; }
  std::string Profile() const override { return m_profilePath; }
  std::string LibPath() const override;
  std::string Author() const override { return m_props.author; }
  std::string ChangeLog() const override { return m_props.changelog; }
  std::string FanArt() const override { return m_props.fanart; }
  std::string Icon() const override { return m_props.icon; };
  std::vector<std::string> Screenshots() const override { return m_props.screenshots; };
  std::string Disclaimer() const override { return m_props.disclaimer; }
  std::string Broken() const override { return m_props.broken; }
  CDateTime InstallDate() const override { return m_props.installDate; }
  CDateTime LastUpdated() const override { return m_props.lastUpdated; }
  CDateTime LastUsed() const override { return m_props.lastUsed; }
  std::string Origin() const override { return m_props.origin; }
  uint64_t PackageSize() const override { return m_props.packageSize; }
  const InfoMap& ExtraInfo() const override { return m_props.extrainfo; }
  const ADDONDEPS& GetDeps() const override { return m_props.dependencies; }

  /*! \brief Check whether the this addon can be configured or not
   \return true if the addon has settings, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool HasSettings() override;

  /*! \brief Check whether the user has configured this addon or not
   \return true if previously saved settings are found, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, GetSetting, UpdateSetting
   */
  virtual bool HasUserSettings();

  /*! \brief Save any user configured settings
   \sa LoadSettings, LoadUserSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  void SaveSettings() override;

  /*! \brief Update a user-configured setting with a new value
   \param key the id of the setting to update
   \param value the value that the setting should take
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  void UpdateSetting(const std::string& key, const std::string& value) override;

  /*! \brief Retrieve a particular settings value
   If a previously configured user setting is available, we return it's value, else we return the default (if available)
   \param key the id of the setting to retrieve
   \return the current value of the setting, or the default if the setting has yet to be configured.
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  std::string GetSetting(const std::string& key) override;

  TiXmlElement* GetSettingsXML() override;

  /*! \brief get the required version of a dependency.
   \param dependencyID the addon ID of the dependency.
   \return the version this addon requires.
   */
  AddonVersion GetDependencyVersion(const std::string &dependencyID) const override;

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  bool MeetsVersion(const AddonVersion &version) const override;
  bool ReloadSettings() override;

  /*! \brief callback for when this add-on is disabled.
   Use to perform any needed actions (e.g. stop a service)
   */
  void OnDisabled() override {};

  /*! \brief callback for when this add-on is enabled.
   Use to perform any needed actions (e.g. start a service)
   */
  void OnEnabled() override {};

  /*! \brief retrieve the running instance of an add-on if it persists while running.
   */
  AddonPtr GetRunningInstance() const override { return AddonPtr(); }

  void OnPreInstall() override {};
  void OnPostInstall(bool update, bool modal) override {};
  void OnPreUnInstall() override {};
  void OnPostUnInstall() override {};

protected:
  /*! \brief Load the default settings and override these with any previously configured user settings
   \param bForce force the load of settings even if they are already loaded (reload)
   \return true if settings exist, false otherwise
   \sa LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool LoadSettings(bool bForce = false);

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

