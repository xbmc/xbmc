/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISettingCallback.h"
#include "ISettingControlCreator.h"
#include "ISettingCreator.h"
#include "ISettingsHandler.h"
#include "ISettingsValueSerializer.h"
#include "Setting.h"
#include "SettingConditions.h"
#include "SettingDefinitions.h"
#include "SettingDependency.h"
#include "threads/SharedSection.h"
#include "utils/logtypes.h"

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

class CSettingCategory;
class CSettingGroup;
class CSettingSection;
class CSettingUpdate;

class TiXmlElement;
class TiXmlNode;

/*!
 \ingroup settings
 \brief Settings manager responsible for initializing, loading and handling
 all settings.
 */
class CSettingsManager : public ISettingCreator,
                         public ISettingControlCreator,
                         private ISettingCallback,
                         private ISettingsHandler
{
public:
  /*!
   \brief Creates a new (uninitialized) settings manager.
   */
  CSettingsManager();
  ~CSettingsManager() override;

  static const uint32_t Version;
  static const uint32_t MinimumSupportedVersion;

  // implementation of ISettingCreator
  std::shared_ptr<CSetting> CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = nullptr) const override;

  // implementation of ISettingControlCreator
  std::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const override;

  uint32_t GetVersion() const { return Version; }
  uint32_t GetMinimumSupportedVersion() const { return MinimumSupportedVersion; }

  /*!
   \brief Try to get the version of the setting definitions/values represented by the given XML element.

   \param root XML element representing setting definitions/values
   \return Version of the setting definitions/values or 0 if no version has been specified
   */
  uint32_t ParseVersion(const TiXmlElement* root) const;

  /*!
   \brief Initializes the settings manager using the setting definitions
   represented by the given XML element.

   \param root XML element representing setting definitions
   \return True if the XML element was successfully deserialized into setting definitions, false otherwise
   */
  bool Initialize(const TiXmlElement *root);
  /*!
   \brief Loads setting values from the given XML element.

   \param root XML element containing setting values
   \param updated Whether some settings were automatically updated
   \param triggerEvents Whether to trigger ISettingCallback methods
   \param loadedSettings A list to fill with all the successfully loaded settings
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const TiXmlElement *root, bool &updated, bool triggerEvents = true, std::map<std::string, std::shared_ptr<CSetting>> *loadedSettings = nullptr);
  /*!
   \brief Saves the setting values using the given serializer.

   \param serializer Settings value serializer to use
   \return True if the setting values were successfully serialized, false otherwise
   */
  bool Save(const ISettingsValueSerializer* serializer, std::string& serializedValues) const;
  /*!
   \brief Unloads the previously loaded setting values.

   The values of all the settings are reset to their default values.
   */
  void Unload();
  /*!
   \brief Clears the complete settings manager.

   This removes all initialized settings, groups, categories and sections and
   returns to the uninitialized state. Any registered callbacks or
   implementations stay registered.
   */
  void Clear();

  /*!
  \brief Loads the setting being represented by the given XML node with the
  given identifier.

  \param node XML node representing the setting to load
  \param settingId Setting identifier
  \return True if the setting was successfully loaded from the given XML node, false otherwise
  */
  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);

  /*!
   \brief Loads the setting being represented by the given XML node with the
   given identifier.

   \param node XML node representing the setting to load
   \param settingId Setting identifier
   \param updated Set to true if the setting's value was updated
   \return True if the setting was successfully loaded from the given XML node, false otherwise
   */
  bool LoadSetting(const TiXmlNode *node, const std::string &settingId, bool &updated);

  /*!
   \brief Tells the settings system that the initialization is complete.

   Setting values can only be loaded after a complete and successful
   initialization of the settings system.
   */
  void SetInitialized();
  /*!
  \brief Returns whether the settings system has been initialized or not.
  */
  bool IsInitialized() const { return m_initialized; }
  /*!
   \brief Tells the settings system that all setting values
   have been loaded.

   This manual trigger is necessary to enable the ISettingCallback methods
   being executed.
   */
  void SetLoaded() { m_loaded = true; }
  /*!
   \brief Returns whether the settings system has been loaded or not.
  */
  bool IsLoaded() const { return m_loaded; }

  /*!
   \brief Adds the given section, its categories, groups and settings.

   This is possible before and after the setting definitions have been
   initialized.
   */
  void AddSection(const std::shared_ptr<CSettingSection>& section);

  /*!
   \brief Adds the given setting to the given group in the given category in
   the given section;

   If the given section has not been added yet, it is added. If the given
   category has not been added to the given section yet, it is added. If the
   given group has not been added to the given category yet, it is added. If
   the given setting has not been added to the given group yet, it is added.

   This is possible before and after the setting definitions have been
   initialized.

   \param setting New setting to be added
   \param section Section the new setting should be added to
   \param category Category the new setting should be added to
   \param group Group the new setting should be added to
   \return True if the setting has been added, false otherwise
   */
  bool AddSetting(const std::shared_ptr<CSetting>& setting,
                  const std::shared_ptr<CSettingSection>& section,
                  const std::shared_ptr<CSettingCategory>& category,
                  const std::shared_ptr<CSettingGroup>& group);

  /*!
   \brief Registers the given ISettingCallback implementation to be triggered
   for the given list of settings.

   \param settingsHandler ISettingsHandler implementation
   \param settingList List of settings to trigger the given ISettingCallback implementation
   */
  void RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList);
  /*!
   \brief Unregisters the given ISettingCallback implementation.

   \param callback ISettingCallback implementation
   */
  void UnregisterCallback(ISettingCallback *callback);

  /*!
   \brief Registers a custom setting type and its ISettingCreator
   implementation.

   When a setting definition for a registered custom setting type is found its
   ISettingCreator implementation is called to create and deserialize the
   setting definition.

   \param settingType String representation of the custom setting type
   \param settingCreator ISettingCreator implementation
   */
  void RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator);

  /*!
   \brief Registers a custom setting control type and its
   ISettingControlCreator implementation

   When a setting control definition for a registered custom setting control
   type is found its ISettingControlCreator implementation is called to create
   and deserialize the setting control definition.

   \param controlType String representation of the custom setting control type
   \param settingControlCreator ISettingControlCreator implementation
   */
  void RegisterSettingControl(const std::string &controlType, ISettingControlCreator *settingControlCreator);

  /*!
   \brief Registers the given ISettingsHandler implementation.

   \param settingsHandler ISettingsHandler implementation
   \param bFront If True, insert the handler in front of other registered handlers, insert at the end otherwise.
   */
  void RegisterSettingsHandler(ISettingsHandler *settingsHandler, bool bFront = false);
  /*!
   \brief Unregisters the given ISettingsHandler implementation.

   \param settingsHandler ISettingsHandler implementation
   */
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);

  /*!
   \brief Registers the given integer setting options filler under the given identifier.

   \param identifier Setting options filler identifier
   \param optionsFiller Integer setting options filler implementation
   */
  void RegisterSettingOptionsFiller(const std::string &identifier, IntegerSettingOptionsFiller optionsFiller);
  /*!
   \brief Registers the given string setting options filler under the given identifier.

   \param identifier Setting options filler identifier
   \param optionsFiller String setting options filler implementation
   */
  void RegisterSettingOptionsFiller(const std::string &identifier, StringSettingOptionsFiller optionsFiller);
  /*!
   \brief Unregisters the setting options filler registered under the given identifier.

   \param identifier Setting options filler identifier
   */
  void UnregisterSettingOptionsFiller(const std::string &identifier);
  /*!
   \brief Gets the implementation of the setting options filler used by the
   given setting.

   \param setting Setting object
   \return Implementation of the setting options filler (either IntegerSettingOptionsFiller or StringSettingOptionsFiller)
   */
  void* GetSettingOptionsFiller(const std::shared_ptr<const CSetting>& setting);

  /*!
   \brief Checks whether any settings have been initialized.
   
   \return True if at least one setting has been initialized, false otherwise*/
  bool HasSettings() const;

  /*!
   \brief Gets the setting with the given identifier.

   \param id Setting identifier
   \return Setting object with the given identifier or nullptr if the identifier is unknown
   */
  std::shared_ptr<CSetting> GetSetting(const std::string &id) const;
  /*!
   \brief Gets the full list of setting sections.

   \return List of setting sections
   */
  std::vector<std::shared_ptr<CSettingSection>> GetSections() const;
  /*!
   \brief Gets the setting section with the given identifier.

   \param section Setting section identifier
   \return Setting section with the given identifier or nullptr if the identifier is unknown
   */
  std::shared_ptr<CSettingSection> GetSection(std::string section) const;
  /*!
   \brief Gets a map of settings (and their dependencies) which depend on
   the setting with the given identifier.

   It is important to note that the returned dependencies are not the
   dependencies of the setting with the given identifier but the settings
   (and their dependencies) which depend on the setting with the given
   identifier.

   \param id Setting identifier
   \return Map of settings (and their dependencies) which depend on the setting with the given identifier
   */
  SettingDependencyMap GetDependencies(const std::string &id) const;
  /*!
   \brief Gets a map of settings (and their dependencies) which depend on
   the given setting.

   It is important to note that the returned dependencies are not the
   dependencies of the given setting but the settings (and their dependencies)
   which depend on the given setting.

   \param setting Setting object
   \return Map of settings (and their dependencies) which depend on the given setting
   */
  SettingDependencyMap GetDependencies(const std::shared_ptr<const CSetting>& setting) const;

  /*!
   \brief Gets the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \return Boolean value of the setting with the given identifier
   */
  bool GetBool(const std::string &id) const;
  /*!
   \brief Gets the integer value of the setting with the given identifier.

   \param id Setting identifier
   \return Integer value of the setting with the given identifier
   */
  int GetInt(const std::string &id) const;
  /*!
   \brief Gets the real number value of the setting with the given identifier.

   \param id Setting identifier
   \return Real number value of the setting with the given identifier
   */
  double GetNumber(const std::string &id) const;
  /*!
   \brief Gets the string value of the setting with the given identifier.

   \param id Setting identifier
   \return String value of the setting with the given identifier
   */
  std::string GetString(const std::string &id) const;
  /*!
   \brief Gets the values of the list setting with the given identifier.

   \param id Setting identifier
   \return List of values of the setting with the given identifier
   */
  std::vector< std::shared_ptr<CSetting> > GetList(const std::string &id) const;

  /*!
   \brief Sets the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \param value Boolean value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetBool(const std::string &id, bool value);
  /*!
   \brief Toggles the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \return True if toggling the boolean value was successful, false otherwise
   */
  bool ToggleBool(const std::string &id);
  /*!
   \brief Sets the integer value of the setting with the given identifier.

   \param id Setting identifier
   \param value Integer value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetInt(const std::string &id, int value);
  /*!
   \brief Sets the real number value of the setting with the given identifier.

   \param id Setting identifier
   \param value Real number value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetNumber(const std::string &id, double value);
  /*!
   \brief Sets the string value of the setting with the given identifier.

   \param id Setting identifier
   \param value String value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetString(const std::string &id, const std::string &value);
  /*!
   \brief Sets the values of the list setting with the given identifier.

   \param id Setting identifier
   \param value Values to set
   \return True if setting the values was successful, false otherwise
   */
  bool SetList(const std::string &id, const std::vector< std::shared_ptr<CSetting> > &value);

  /*!
   \brief Sets the value of the setting to its default.

   \param id Setting identifier
   \return True if setting the value to its default was successful, false otherwise
   */
  bool SetDefault(const std::string &id);
  /*!
  \brief Sets the value of all settings to their default.
  */
  void SetDefaults();

  /*!
   \brief Gets the setting conditions manager used by the settings manager.

   \return Setting conditions manager used by the settings manager.
   */
  const CSettingConditionsManager& GetConditions() const { return m_conditions; }
  /*!
   \brief Adds the given static condition.

   A static condition is just a string. If a static condition is evaluated,
   the result depends on whether the condition's value is defined or not.

   \param condition Static condition string/value
   */
  void AddCondition(const std::string &condition);
  /*!
   \brief Adds the given dynamic condition.

   A dynamic condition has an identifier and an implementation which is
   triggered when the condition is evaluated.

   \param identifier Identifier of the dynamic condition
   \param condition Implementation of the dynamic condition
   \param data Opaque data pointer, will be passed back to SettingConditionCheck function
   */
  void AddDynamicCondition(const std::string &identifier, SettingConditionCheck condition, void *data = nullptr);

  /*!
   \brief Removes the given dynamic condition.

   \param identifier Identifier of the dynamic condition
   */
  void RemoveDynamicCondition(const std::string &identifier);

private:
  // implementation of ISettingCallback
  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;
  void OnSettingPropertyChanged(const std::shared_ptr<const CSetting>& setting,
                                const char* propertyName) override;

  // implementation of ISettingsHandler
  bool OnSettingsLoading() override;
  void OnSettingsLoaded() override;
  void OnSettingsUnloaded() override;
  bool OnSettingsSaving() const override;
  void OnSettingsSaved() const override;
  void OnSettingsCleared() override;

  bool Serialize(TiXmlNode *parent) const;
  bool Deserialize(const TiXmlNode *node, bool &updated, std::map<std::string, std::shared_ptr<CSetting>> *loadedSettings = nullptr);

  bool LoadSetting(const TiXmlNode* node, const std::shared_ptr<CSetting>& setting, bool& updated);
  bool UpdateSetting(const TiXmlNode* node,
                     const std::shared_ptr<CSetting>& setting,
                     const CSettingUpdate& update);
  void UpdateSettingByDependency(const std::string &settingId, const CSettingDependency &dependency);
  void UpdateSettingByDependency(const std::string &settingId, SettingDependencyType dependencyType);

  void AddSetting(const std::shared_ptr<CSetting>& setting);

  void ResolveReferenceSettings(const std::shared_ptr<CSettingSection>& section);
  void CleanupIncompleteSettings();

  enum class SettingOptionsFillerType {
    Unknown = 0,
    Integer,
    String
  };

  void RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type);

  using CallbackSet = std::set<ISettingCallback *>;
  struct Setting {
    std::shared_ptr<CSetting> setting;
    SettingDependencyMap dependencies;
    std::set<std::string> children;
    CallbackSet callbacks;
    std::unordered_set<std::string> references;
  };

  using SettingMap = std::map<std::string, Setting>;

  /*!
   * \brief Refresh the visibility and enable status of a given setting
   *
   * \details A setting might have its visibility/enable status bound to complex conditions and, at the same time, depend
   * on other settings. When those settings change, the visibility/enable status need to be refreshed (i.e. the complex condition must be re-evaluated)
   *
   * \param setting Setting object
  */
  void RefreshVisibilityAndEnableStatus(const std::shared_ptr<const CSetting>& setting);

  void ResolveSettingDependencies(const std::shared_ptr<CSetting>& setting);
  void ResolveSettingDependencies(const Setting& setting);

  SettingMap::const_iterator FindSetting(std::string settingId) const;
  SettingMap::iterator FindSetting(std::string settingId);
  std::pair<SettingMap::iterator, bool> InsertSetting(std::string settingId, const Setting& setting);

  bool m_initialized = false;
  bool m_loaded = false;

  SettingMap m_settings;
  using SettingSectionMap = std::map<std::string, std::shared_ptr<CSettingSection>>;
  SettingSectionMap m_sections;

  using SettingCreatorMap = std::map<std::string, ISettingCreator*>;
  SettingCreatorMap m_settingCreators;

  using SettingControlCreatorMap = std::map<std::string, ISettingControlCreator*>;
  SettingControlCreatorMap m_settingControlCreators;

  using SettingsHandlers = std::vector<ISettingsHandler*>;
  SettingsHandlers m_settingsHandlers;

  CSettingConditionsManager m_conditions;

  struct SettingOptionsFiller {
    void *filler;
    SettingOptionsFillerType type;
  };
  using SettingOptionsFillerMap = std::map<std::string, SettingOptionsFiller>;
  SettingOptionsFillerMap m_optionsFillers;

  mutable CSharedSection m_critical;
  mutable CSharedSection m_settingsCritical;

  Logger m_logger;
};
