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

typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_extension_t cp_extension_t;

namespace ADDON
{
  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;

// utils
const std::string   TranslateType(const TYPE &type, bool pretty=false);
const std::string   GetIcon(const TYPE &type);
      TYPE          TranslateType(const std::string &string);

void OnEnabled(const std::string& id);
void OnDisabled(const std::string& id);
void OnPreInstall(const AddonPtr& addon);
void OnPostInstall(const AddonPtr& addon, bool update, bool modal);
void OnPreUnInstall(const AddonPtr& addon);
void OnPostUnInstall(const AddonPtr& addon);

class AddonProps : public ISerializable
{
public:
  AddonProps(const std::string &id, TYPE type, const std::string &versionstr, const std::string &minversionstr)
    : id(id)
    , type(type)
    , version(versionstr)
    , minversion(minversionstr)
    , stars(0)
  {
  }

  virtual ~AddonProps() {}

  AddonProps(const cp_extension_t *ext);
  AddonProps(const cp_plugin_info_t *plugin);

  bool operator==(const AddonProps &rhs)
  { 
    return    (*this).id == rhs.id
           && (*this).type == rhs.type
           && (*this).version == rhs.version;
  }
  
  void Serialize(CVariant &variant) const;

  std::string id;
  TYPE type;
  AddonVersion version;
  AddonVersion minversion;
  std::string name;
  std::string license;
  std::string summary;
  std::string description;
  std::string path;
  std::string libname;
  std::string author;
  std::string source;
  std::string icon;
  std::string disclaimer;
  std::string changelog;
  std::string fanart;
  ADDONDEPS dependencies;
  std::string broken;
  InfoMap    extrainfo;
  int        stars;
private:
  void BuildDependencies(const cp_plugin_info_t *plugin);
};

typedef std::vector<class AddonProps> VECADDONPROPS;

class CAddon : public IAddon
{
public:
  CAddon(const AddonProps &addonprops);
  CAddon(const cp_extension_t *ext);
  CAddon(const cp_plugin_info_t *plugin);
  virtual ~CAddon() {}
  virtual AddonPtr Clone() const;

  /*! \brief Check whether the this addon can be configured or not
   \return true if the addon has settings, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool HasSettings();

  /*! \brief Check whether the user has configured this addon or not
   \return true if previously saved settings are found, false otherwise
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, GetSetting, UpdateSetting
   */
  bool HasUserSettings();

  /*! \brief Save any user configured settings
   \sa LoadSettings, LoadUserSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual void SaveSettings();

  /*! \brief Update a user-configured setting with a new value
   \param key the id of the setting to update
   \param value the value that the setting should take
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  void UpdateSetting(const std::string& key, const std::string& value);

  /*! \brief Retrieve a particular settings value
   If a previously configured user setting is available, we return it's value, else we return the default (if available)
   \param key the id of the setting to retrieve
   \return the current value of the setting, or the default if the setting has yet to be configured.
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  virtual std::string GetSetting(const std::string& key);

  TiXmlElement* GetSettingsXML();
  virtual std::string GetString(uint32_t id);

  // properties
  TYPE Type() const { return m_props.type; }
  bool IsType(TYPE type) const { return type == m_props.type; }
  AddonProps Props() const { return m_props; }
  AddonProps& Props() { return m_props; }
  const std::string ID() const { return m_props.id; }
  const std::string Name() const { return m_props.name; }
  /*! This lies. Ask CAddonMgr */
  bool Enabled() const { return true; }
  virtual bool IsInUse() const { return false; };
  const AddonVersion Version() const { return m_props.version; }
  const AddonVersion MinVersion() const { return m_props.minversion; }
  const std::string Summary() const { return m_props.summary; }
  const std::string Description() const { return m_props.description; }
  const std::string Path() const { return m_props.path; }
  const std::string Profile() const { return m_profile; }
  const std::string LibPath() const;
  const std::string Author() const { return m_props.author; }
  const std::string ChangeLog() const { return m_props.changelog; }
  const std::string FanArt() const { return m_props.fanart; }
  const std::string Icon() const;
  int Stars() const { return m_props.stars; }
  const std::string Disclaimer() const { return m_props.disclaimer; }
  const InfoMap &ExtraInfo() const { return m_props.extrainfo; }
  const ADDONDEPS &GetDeps() const { return m_props.dependencies; }

  /*! \brief get the required version of a dependency.
   \param dependencyID the addon ID of the dependency.
   \return the version this addon requires.
   */
  AddonVersion GetDependencyVersion(const std::string &dependencyID) const;

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  bool MeetsVersion(const AddonVersion &version) const;
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
  virtual bool CanInstall(const std::string& referer) { return true; }
protected:
  friend class CAddonCallbacksAddon;

  CAddon(const CAddon &rhs); // protected as all copying is handled by Clone()
  virtual void BuildLibName(const cp_extension_t *ext = NULL);

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
  bool LoadUserSettings();

  /*! \brief Parse settings from an XML document
   \param doc XML document to parse for settings
   \param loadDefaults if true, the default attribute is used and settings are reset prior to parsing, else the value attribute is used.
   \return true if settings are loaded, false otherwise
   \sa SettingsToXML
   */
  bool SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults = false);

  /*! \brief Parse settings into an XML document
   \param doc XML document to receive the settings
   \sa SettingsFromXML
   */
  void SettingsToXML(CXBMCTinyXML &doc) const;

  CXBMCTinyXML      m_addonXmlDoc;
  std::string       m_strLibName;
  bool              m_settingsLoaded;
  bool              m_userSettingsLoaded;

private:
  friend class CAddonMgr;
  AddonProps m_props;
  std::string        m_userSettingsPath;
  void BuildProfilePath();

  virtual bool LoadStrings();
  virtual void ClearStrings();

  bool m_hasStrings;
  bool m_checkedStrings;
  bool m_hasSettings;

  std::string m_profile;
  CLocalizeStrings  m_strings;
  std::map<std::string, std::string> m_settings;
};

class CAddonLibrary : public CAddon
{
public:
  CAddonLibrary(const AddonProps &props);
  CAddonLibrary(const cp_extension_t *ext);

  virtual AddonPtr Clone() const;

private:
  TYPE SetAddonType();
  const TYPE m_addonType; // addon type this library enhances
};

}; /* namespace ADDON */

