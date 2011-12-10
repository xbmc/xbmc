#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IAddon.h"
#include "addons/AddonVersion.h"
#include "tinyXML/tinyxml.h"
#include "Util.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "addons/AddonCallbacksAddon.h"

class CURL;
class TiXmlElement;

typedef struct cp_plugin_info_t cp_plugin_info_t;
typedef struct cp_extension_t cp_extension_t;

namespace ADDON
{
  typedef std::vector<AddonPtr> VECADDONS;
  typedef std::vector<AddonPtr>::iterator IVECADDONS;

// utils
const CStdString    TranslateType(const TYPE &type, bool pretty=false);
const CStdString    GetIcon(const TYPE &type);
      TYPE          TranslateType(const CStdString &string);
const CStdString    UpdateVideoScraper(const CStdString &scraper);
const CStdString    UpdateMusicScraper(const CStdString &scraper);

class AddonProps
{
public:
  AddonProps(const CStdString &id, TYPE type, const CStdString &versionstr, const CStdString &minversionstr)
    : id(id)
    , type(type)
    , version(versionstr)
    , minversion(minversionstr)
    , stars(0)
  {
  }

  AddonProps(const cp_extension_t *ext);
  AddonProps(const cp_plugin_info_t *plugin);

  bool operator==(const AddonProps &rhs)
  { 
    return    (*this).id == rhs.id
           && (*this).type == rhs.type
           && (*this).version == rhs.version;
  }

  CStdString id;
  TYPE type;
  AddonVersion version;
  AddonVersion minversion;
  CStdString name;
  CStdString parent;
  CStdString license;
  CStdString summary;
  CStdString description;
  CStdString path;
  CStdString libname;
  CStdString author;
  CStdString source;
  CStdString icon;
  CStdString disclaimer;
  CStdString changelog;
  CStdString fanart;
  ADDONDEPS dependencies;
  CStdString broken;
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
  virtual AddonPtr Clone(const AddonPtr& parent) const;

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
  void UpdateSetting(const CStdString& key, const CStdString& value);

  /*! \brief Retrieve a particular settings value
   If a previously configured user setting is available, we return it's value, else we return the default (if available)
   \param key the id of the setting to retrieve
   \return the current value of the setting, or the default if the setting has yet to be configured.
   \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  virtual CStdString GetSetting(const CStdString& key);

  TiXmlElement* GetSettingsXML();
  virtual CStdString GetString(uint32_t id);

  // properties
  TYPE Type() const { return m_props.type; }
  bool IsType(TYPE type) const { return type == m_props.type; }
  AddonProps Props() const { return m_props; }
  AddonProps& Props() { return m_props; }
  const CStdString ID() const { return m_props.id; }
  const CStdString Name() const { return m_props.name; }
  bool Enabled() const { return m_enabled; }
  virtual bool IsInUse() const { return false; };
  const AddonVersion Version() const { return m_props.version; }
  const AddonVersion MinVersion() const { return m_props.minversion; }
  const CStdString Summary() const { return m_props.summary; }
  const CStdString Description() const { return m_props.description; }
  const CStdString Path() const { return m_props.path; }
  const CStdString Profile() const { return m_profile; }
  const CStdString LibPath() const;
  const CStdString Author() const { return m_props.author; }
  const CStdString ChangeLog() const { return m_props.changelog; }
  const CStdString FanArt() const { return m_props.fanart; }
  const CStdString Icon() const;
  int Stars() const { return m_props.stars; }
  const CStdString Disclaimer() const { return m_props.disclaimer; }
  const InfoMap &ExtraInfo() const { return m_props.extrainfo; }
  const ADDONDEPS &GetDeps() const { return m_props.dependencies; }

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  bool MeetsVersion(const AddonVersion &version) const;
  virtual bool ReloadSettings();

protected:
  friend class CAddonCallbacksAddon;

  CAddon(const CAddon&); // protected as all copying is handled by Clone()
  CAddon(const CAddon&, const AddonPtr&);
  const AddonPtr Parent() const { return m_parent; }
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
  bool SettingsFromXML(const TiXmlDocument &doc, bool loadDefaults = false);

  /*! \brief Parse settings into an XML document
   \param doc XML document to receive the settings
   \sa SettingsFromXML
   */
  void SettingsToXML(TiXmlDocument &doc) const;

  TiXmlDocument     m_addonXmlDoc;
  CStdString        m_strLibName;
  bool              m_settingsLoaded;
  bool              m_userSettingsLoaded;

private:
  friend class CAddonMgr;
  AddonProps m_props;
  const AddonPtr    m_parent;
  CStdString        m_userSettingsPath;
  void BuildProfilePath();

  virtual bool IsAddonLibrary() { return false; }

  void Enable() { LoadStrings(); m_enabled = true; }
  void Disable() { m_enabled = false; ClearStrings();}

  virtual bool LoadStrings();
  virtual void ClearStrings();
  bool m_hasStrings;
  bool m_checkedStrings;
  bool m_hasSettings;

  CStdString  m_profile;
  bool        m_enabled;
  CLocalizeStrings  m_strings;
  std::map<CStdString, CStdString> m_settings;
};

class CAddonLibrary : public CAddon
{
public:
  CAddonLibrary(const AddonProps &props);
  CAddonLibrary(const cp_extension_t *ext);

private:
  virtual bool IsAddonLibrary() { return true; }
  TYPE SetAddonType();
  const TYPE m_addonType; // addon type this library enhances
};

}; /* namespace ADDON */

