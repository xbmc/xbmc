#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>
#include <set>
#include <vector>

#include "ISettingCallback.h"
#include "ISettingControlCreator.h"
#include "ISettingCreator.h"
#include "ISettingsHandler.h"
#include "ISubSettings.h"
#include "Setting.h"
#include "SettingConditions.h"
#include "SettingDefinitions.h"
#include "SettingDependency.h"
#include "threads/SharedSection.h"

class CSettingSection;
class CSettingUpdate;

class TiXmlElement;
class TiXmlNode;

/*!
 \ingroup settings
 \brief Settings manager responsible for initializing, loading and handling
 all settings.
 */
class CSettingsManager : public ISettingCreator, public ISettingControlCreator,
                         private ISettingCallback,
                         private ISettingsHandler, private ISubSettings
{
public:
  /*!
   \brief Creates a new (uninitialized) settings manager.
   */
  CSettingsManager();
  virtual ~CSettingsManager();

  // implementation of ISettingCreator
  virtual CSetting* CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = NULL) const;

  // implementation of ISettingControlCreator
  virtual ISettingControl* CreateControl(const std::string &controlType) const;

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
  bool Load(const TiXmlElement *root, bool &updated, bool triggerEvents = true, std::map<std::string, CSetting*> *loadedSettings = NULL);
  /*!
   \brief Saves the setting values to the given XML node.

   \param root XML node
   \return True if the setting values were successfully saved, false otherwise
   */
  virtual bool Save(TiXmlNode *root) const;
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
   \brief Tells the settings system that all setting values
   have been loaded.

   This manual trigger is necessary to enable the ISettingCallback methods
   being executed.
   */
  void SetLoaded() { m_loaded = true; }

  void AddSection(CSettingSection *section);

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
   */
  void RegisterSettingsHandler(ISettingsHandler *settingsHandler);
  /*!
   \brief Unregisters the given ISettingsHandler implementation.

   \param settingsHandler ISettingsHandler implementation
   */
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);

  /*!
   \brief Registers the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void RegisterSubSettings(ISubSettings *subSettings);
  /*!
   \brief Unregisters the given ISubSettings implementation.

   \param subSettings ISubSettings implementation
   */
  void UnregisterSubSettings(ISubSettings *subSettings);

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
  void* GetSettingOptionsFiller(const CSetting *setting);

  /*!
   \brief Gets the setting with the given identifier.

   \param id Setting identifier
   \return Setting object with the given identifier or NULL if the identifier is unknown
   */
  CSetting* GetSetting(const std::string &id) const;
  /*!
   \brief Gets the full list of setting sections.

   \return List of setting sections
   */
  std::vector<CSettingSection*> GetSections() const;
  /*!
   \brief Gets the setting section with the given identifier.

   \param section Setting section identifier
   \return Setting section with the given identifier or NULL if the identifier is unknown
   */
  CSettingSection* GetSection(const std::string &section) const;
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
  SettingDependencyMap GetDependencies(const CSetting *setting) const;

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
  void AddCondition(const std::string &identifier, SettingConditionCheck condition, void *data = NULL);

private:
  // implementation of ISettingCallback
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);
  virtual void OnSettingPropertyChanged(const CSetting *setting, const char *propertyName);

  // implementation of ISettingsHandler
  virtual bool OnSettingsLoading();
  virtual void OnSettingsLoaded();
  virtual void OnSettingsUnloaded();
  virtual bool OnSettingsSaving() const;
  virtual void OnSettingsSaved() const;
  virtual void OnSettingsCleared();

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode *settings);

  bool Serialize(TiXmlNode *parent) const;
  bool Deserialize(const TiXmlNode *node, bool &updated, std::map<std::string, CSetting*> *loadedSettings = NULL);

  bool LoadSetting(const TiXmlNode *node, CSetting *setting, bool &updated);
  bool UpdateSetting(const TiXmlNode *node, CSetting *setting, const CSettingUpdate& update);
  void UpdateSettingByDependency(const std::string &settingId, const CSettingDependency &dependency);
  void UpdateSettingByDependency(const std::string &settingId, SettingDependencyType dependencyType);

  typedef enum {
    SettingOptionsFillerTypeNone = 0,
    SettingOptionsFillerTypeInteger,
    SettingOptionsFillerTypeString
  } SettingOptionsFillerType;

  void RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type);

  typedef std::set<ISettingCallback *> CallbackSet;
  typedef struct {
    CSetting *setting;
    SettingDependencyMap dependencies;
    std::set<std::string> children;
    CallbackSet callbacks;
  } Setting;

  bool m_initialized;
  bool m_loaded;

  typedef std::map<std::string, Setting> SettingMap;
  SettingMap m_settings;
  typedef std::map<std::string, CSettingSection*> SettingSectionMap;
  SettingSectionMap m_sections;

  typedef std::map<std::string, ISettingCreator*> SettingCreatorMap;
  SettingCreatorMap m_settingCreators;

  typedef std::map<std::string, ISettingControlCreator*> SettingControlCreatorMap;
  SettingControlCreatorMap m_settingControlCreators;

  std::set<ISubSettings*> m_subSettings;
  typedef std::vector<ISettingsHandler*> SettingsHandlers;
  SettingsHandlers m_settingsHandlers;

  CSettingConditionsManager m_conditions;

  typedef struct {
    void *filler;
    SettingOptionsFillerType type;
  } SettingOptionsFiller;
  typedef std::map<std::string, SettingOptionsFiller> SettingOptionsFillerMap;
  SettingOptionsFillerMap m_optionsFillers;

  CSharedSection m_critical;
  CSharedSection m_settingsCritical;
};
