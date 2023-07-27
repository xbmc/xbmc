/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class CXBMCTinyXML;

namespace ADDON
{

enum class AddonType;
class CAddonType;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

void OnPreInstall(const AddonPtr& addon);
void OnPostInstall(const AddonPtr& addon, bool update, bool modal);
void OnPreUnInstall(const AddonPtr& addon);
void OnPostUnInstall(const AddonPtr& addon);

class CAddon : public IAddon
{
public:
  explicit CAddon(const AddonInfoPtr& addonInfo, AddonType addonType);
  ~CAddon() override = default;

  /**
   * @brief To get the main type of this addon
   *
   * This is the first type defined in **addon.xml** and can be different to the
   * on @ref Type() defined type.
   *
   * @return The used main type of addon
   */
  AddonType MainType() const override;

  /**
   * @brief To get the on this CAddon class processed addon type
   *
   * @return For this class used addon type
   */
  AddonType Type() const override { return m_type; }

  /**
   * @brief To check complete addon (not only this) contains a type
   *
   * @note This can be overridden by a child e.g. plugin to check for subtype
   * e.g. video or music.
   *
   * @param[in] type The to checked type identifier
   * @return true in case the wanted type is supported, false if not
   */
  bool HasType(AddonType type) const override;

  /**
   * @brief To check complete addon (not only this) has a specific type
   * defined in its first extension point including the provided subcontent
   * e.g. video or audio
   *
   * @param[in] type Type identifier to be checked
   * @return true in case the wanted type is the main type, false if not
   */
  bool HasMainType(AddonType type) const override;

  /**
   * @brief The get for given addon type information and extension data
   *
   * @param[in] type The wanted type data
   * @return addon type class with @ref CAddonExtensions as information
   *
   * @note This function return never a "nullptr", in case the wanted type is
   * not supported, becomes a dummy of @ref CAddonType given.
   *
   * ------------------------------------------------------------------------
   *
   * **Example:**
   * ~~~~~~~~~~~~~{.cpp}
   * // To get e.g. <extension ... name="blablabla" /> from addon.xml
   * std::string name = Type(ADDON_...)->GetValue("@name").asString();
   * ~~~~~~~~~~~~~
   *
   */
  const CAddonType* Type(AddonType type) const;

  std::string ID() const override;
  std::string Name() const override;
  bool IsInUse() const override { return false; }
  bool IsBinary() const override;
  CAddonVersion Version() const override;
  CAddonVersion MinVersion() const override;
  std::string Summary() const override;
  std::string Description() const override;
  std::string Path() const override;
  std::string Profile() const override;
  std::string LibPath() const override;
  std::string Author() const override;
  std::string ChangeLog() const override;
  std::string Icon() const override;
  ArtMap Art() const override;
  std::vector<std::string> Screenshots() const override;
  std::string Disclaimer() const override;
  AddonLifecycleState LifecycleState() const override;
  std::string LifecycleStateDescription() const override;
  CDateTime InstallDate() const override;
  CDateTime LastUpdated() const override;
  CDateTime LastUsed() const override;
  std::string Origin() const override;
  std::string OriginName() const override;
  uint64_t PackageSize() const override;
  const InfoMap& ExtraInfo() const override;
  const std::vector<DependencyInfo>& GetDependencies() const override;
  std::string FanArt() const override;

  /*!
   * \brief Check add-on for support from independent work instances.
   *
   * \return true if the add-on supports individual add-on instances, false otherwise
   */
  bool SupportsMultipleInstances() const override;

  /*!
   * \brief Return the used instance path type of the add-on type.
   *
   * \return The route used to instance handling, @ref AddonInstanceUse::NONE if not supported.
   */
  AddonInstanceSupport InstanceUseType() const override;

  /*!
   * \brief Gives active, independently working instance identifiers for this add-on.
   *
   * This function is supported if add-on type has defined
   * @ref AddonInstanceUse::BY_SETTINGS and the associated settings
   * are available.
   *
   * \return List of active instance identifiers.
   */
  std::vector<AddonInstanceId> GetKnownInstanceIds() const override;

  /*!
   * \brief Check whether the add-on supports individual settings per add-on instance.
   *
   * This function is supported if add-on type has defined
   * @ref AddonInstanceUse::BY_SETTINGS
   *
   * \return true if the add-on supports individual settings per add-on instance, false otherwise
   */
  bool SupportsInstanceSettings() const override;

  /*!
   * \brief Delete selected instance settings from storage.
   *
   * The related instance-settings-[0-9...].xml file will be deleted by this method.
   *
   * \param[in] instance Instance identifier to use.
   * \return true on success, false otherwise.
   */
  bool DeleteInstanceSettings(AddonInstanceId instance) override;

  /*!
   * \brief Check whether this add-on can be configured by the user.
   *
   * \return true if the add-on has settings, false otherwise
   */
  bool CanHaveAddonOrInstanceSettings() override;

  /*!
   * \brief Check whether this add-on can be configured by the user.
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if the add-on has settings, false otherwise
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool HasSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Check whether the user has configured this add-on or not.
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if previously saved settings are found, false otherwise
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, GetSetting, UpdateSetting
   */
  bool HasUserSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Save any user configured settings
   *
   * \param[in] instance Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *                     to denote global add-on settings from settings.xml.
   * \return true if the operation was successful, false otherwise
   *
   * \sa LoadSettings, LoadUserSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool SaveSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Update a user-configured setting with a new value.
   *
   * \param[in] key the id of the setting to update
   * \param[in] value the value that the setting should take
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  void UpdateSetting(const std::string& key,
                     const std::string& value,
                     AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Update a user-configured setting with a new boolean value.
   *
   * \param[in] key the id of the setting to update
   * \param[in] value the value that the setting should take
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  bool UpdateSettingBool(const std::string& key,
                         bool value,
                         AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Update a user-configured setting with a new integer value.
   *
   * \param[in] key the id of the setting to update
   * \param[in] value the value that the setting should take
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  bool UpdateSettingInt(const std::string& key,
                        int value,
                        AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Update a user-configured setting with a new number value.
   *
   * \param[in] key the id of the setting to update
   * \param[in] value the value that the setting should take
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  bool UpdateSettingNumber(const std::string& key,
                           double value,
                           AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Update a user-configured setting with a new string value.
   *
   * \param[in] key the id of the setting to update
   * \param[in] value the value that the setting should take
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting
   */
  bool UpdateSettingString(const std::string& key,
                           const std::string& value,
                           AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Retrieve a particular settings value.
   *
   * If a previously configured user setting is available, we return it's value, else we return the default (if available).
   *
   * \param[in] key the id of the setting to retrieve
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return the current value of the setting, or the default if the setting has yet to be configured.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  std::string GetSetting(const std::string& key, AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Retrieve a particular settings value as boolean.
   *
   * If a previously configured user setting is available, we return it's value, else we return the default (if available).
   *
   * \param[in] key the id of the setting to retrieve
   * \param[out] value the current value of the setting, or the default if the setting has yet to be configured
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if the setting's value was retrieved, false otherwise.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  bool GetSettingBool(const std::string& key,
                      bool& value,
                      AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Retrieve a particular settings value as integer.
   *
   * If a previously configured user setting is available, we return it's value, else we return the default (if available)
   *
   * \param[in] key the id of the setting to retrieve
   * \param[out] value the current value of the setting, or the default if the setting has yet to be configured
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if the setting's value was retrieved, false otherwise.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  bool GetSettingInt(const std::string& key,
                     int& value,
                     AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Retrieve a particular settings value as number.
   *
   * If a previously configured user setting is available, we return it's value, else we return the default (if available)
   *
   * \param[in] key the id of the setting to retrieve
   * \param[out] value the current value of the setting, or the default if the setting has yet to be configured
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if the setting's value was retrieved, false otherwise.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  bool GetSettingNumber(const std::string& key,
                        double& value,
                        AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*!
   * \brief Retrieve a particular settings value as string
   *
   * If a previously configured user setting is available, we return it's value, else we return the default (if available)
   *
   * \param[in] key the id of the setting to retrieve
   * \param[out] value the current value of the setting, or the default if the setting has yet to be configured
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if the setting's value was retrieved, false otherwise.
   *
   * \sa LoadSettings, LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, UpdateSetting
   */
  bool GetSettingString(const std::string& key,
                        std::string& value,
                        AddonInstanceId id = ADDON_SETTINGS_ID) override;

  std::shared_ptr<CAddonSettings> GetSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*! \brief get the required version of a dependency.
   \param dependencyID the addon ID of the dependency.
   \return the version this addon requires.
   */
  CAddonVersion GetDependencyVersion(const std::string& dependencyID) const override;

  /*! \brief return whether or not this addon satisfies the given version requirements
   \param version the version to meet.
   \return true if  min_version <= version <= current_version, false otherwise.
   */
  bool MeetsVersion(const CAddonVersion& versionMin, const CAddonVersion& version) const override;

  bool ReloadSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  void ResetSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override;

  /*! \brief retrieve the running instance of an add-on if it persists while running.
   */
  AddonPtr GetRunningInstance() const override { return AddonPtr(); }

  void OnPreInstall() override{};
  void OnPostInstall(bool update, bool modal) override{};
  void OnPreUnInstall() override{};
  void OnPostUnInstall() override{};

protected:
  /*!
   * \brief Whether or not the settings have been initialized.
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings initialize was successfull
   */
  virtual bool SettingsInitialized(AddonInstanceId id = ADDON_SETTINGS_ID) const;

  /*!
   * \brief Whether or not the settings have been loaded.
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings are loaded correct
   */
  virtual bool SettingsLoaded(AddonInstanceId id = ADDON_SETTINGS_ID) const;

  /*!
   * \brief Load the default settings and override these with any previously configured user settings
   *
   * \param[in] bForce force the load of settings even if they are already loaded (reload)
   * \param[in] loadUserSettings whether or not to load user settings
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings exist, false otherwise
   *
   * \sa LoadUserSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  bool LoadSettings(bool bForce, bool loadUserSettings, AddonInstanceId id = ADDON_SETTINGS_ID);

  /*!
   * \brief Load the user settings
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if user settings exist, false otherwise
   *
   * \sa LoadSettings, SaveSettings, HasSettings, HasUserSettings, GetSetting, UpdateSetting
   */
  virtual bool LoadUserSettings(AddonInstanceId id = ADDON_SETTINGS_ID);

  /*!
   * \brief Whether there are settings to be saved
   *
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings has to save
   *
   * \sa SaveSettings
   */
  virtual bool HasSettingsToSave(AddonInstanceId id = ADDON_SETTINGS_ID) const;

  /*!
   * \brief Parse settings from an XML document
   *
   * \param[in] doc XML document to parse for settings
   * \param[in] loadDefaults if true, the default attribute is used and settings are reset prior to parsing, else the value attribute is used.
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings are loaded, false otherwise
   *
   * \sa SettingsToXML
   */
  virtual bool SettingsFromXML(const CXBMCTinyXML& doc,
                               bool loadDefaults,
                               AddonInstanceId id = ADDON_SETTINGS_ID);

  /*!
   * \brief Write settings into an XML document
   *
   * \param[out] doc XML document to receive the settings
   * \param[in] id Instance identifier to use, use @ref ADDON_SETTINGS_ID
   *               to denote global add-on settings from settings.xml.
   * \return true if settings are saved, false otherwise
   *
   * \sa SettingsFromXML
   */
  virtual bool SettingsToXML(CXBMCTinyXML& doc, AddonInstanceId id = ADDON_SETTINGS_ID) const;

  const AddonInfoPtr m_addonInfo;

private:
  struct CSettingsData
  {
    bool m_loadSettingsFailed{false};
    bool m_hasUserSettings{false};
    std::string m_addonSettingsPath;
    std::string m_userSettingsPath;
    std::shared_ptr<CAddonSettings> m_addonSettings;
  };

  bool InitSettings(AddonInstanceId id);
  std::shared_ptr<CAddonSettings> FindInstanceSettings(AddonInstanceId id) const;

  mutable std::unordered_map<AddonInstanceId, CSettingsData> m_settings;
  const AddonType m_type;
};

}; // namespace ADDON
