#pragma once
/*
 *      Copyright (C) 2016 Team XBMC
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

#include <set>
#include <string>
#include <vector>

#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

class CSetting;
class CSettingSection;
class CSettingsManager;
class CVariant;
class CXBMCTinyXML;
class TiXmlElement;

/*!
 \brief Basic wrapper around CSettingsManager providing the framework for
 properly setting up the settings manager and registering all the callbacks,
 handlers and custom setting types.
 \sa CSettingsManager
 */
class CSettingsBase
{
public:
  virtual ~CSettingsBase();

  CSettingsManager* GetSettingsManager() const { return m_settingsManager; }

  /*!
   \brief Initializes the setting system with the generic
   settings definition and platform specific setting definitions.

   \return True if the initialization was successful, false otherwise
   */
  virtual bool Initialize();
  /*!
   \brief Returns whether the settings system has been initialized or not.
  */
  virtual bool IsInitialized() const;
  /*!
   \brief Loads the setting values.

   \return True if the setting values are successfully loaded, false otherwise
   */
  virtual bool Load() = 0;
  /*!
   \brief Tells the settings system that all setting values
   have been loaded.

   This manual trigger is necessary to enable the ISettingCallback methods
   being executed.
   */
  virtual void SetLoaded();
  /*!
   \brief Returns whether the settings system has been loaded or not.
   */
  virtual bool IsLoaded() const;
  /*!
   \brief Saves the setting values.

   \return True if the setting values were successfully saved, false otherwise
   */
  virtual bool Save() = 0;
  /*!
   \brief Unloads the previously loaded setting values.

   The values of all the settings are reset to their default values.
   */
  virtual void Unload();
  /*!
   \brief Uninitializes the settings system.

   Unregisters all previously registered callbacks and destroys all setting
   objects.
   */
  virtual void Uninitialize();

  /*!
   \brief Registers the given ISettingCallback implementation for the given
   set of settings.

   \param callback ISettingCallback implementation
   \param settingList List of setting identifiers for which the given callback shall be triggered
   */
  void RegisterCallback(ISettingCallback* callback, const std::set<std::string>& settingList);
  /*!
   \brief Unregisters the given ISettingCallback implementation.

   \param callback ISettingCallback implementation
   */
  void UnregisterCallback(ISettingCallback* callback);

  /*!
   \brief Gets the setting with the given identifier.

   \param id Setting identifier
   \return Setting object with the given identifier or NULL if the identifier is unknown
   */
  CSetting* GetSetting(const std::string& id) const;
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
  CSettingSection* GetSection(const std::string& section) const;

  /*!
   \brief Gets the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \return Boolean value of the setting with the given identifier
   */
  bool GetBool(const std::string& id) const;
  /*!
   \brief Gets the integer value of the setting with the given identifier.

   \param id Setting identifier
   \return Integer value of the setting with the given identifier
   */
  int GetInt(const std::string& id) const;
  /*!
   \brief Gets the real number value of the setting with the given identifier.

   \param id Setting identifier
   \return Real number value of the setting with the given identifier
   */
  double GetNumber(const std::string& id) const;
  /*!
   \brief Gets the string value of the setting with the given identifier.

   \param id Setting identifier
   \return String value of the setting with the given identifier
   */
  std::string GetString(const std::string& id) const;
  /*!
   \brief Gets the values of the list setting with the given identifier.

   \param id Setting identifier
   \return List of values of the setting with the given identifier
   */
  std::vector<CVariant> GetList(const std::string& id) const;

  /*!
   \brief Sets the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \param value Boolean value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetBool(const std::string& id, bool value);
  /*!
   \brief Toggles the boolean value of the setting with the given identifier.

   \param id Setting identifier
   \return True if toggling the boolean value was successful, false otherwise
   */
  bool ToggleBool(const std::string& id);
  /*!
   \brief Sets the integer value of the setting with the given identifier.

   \param id Setting identifier
   \param value Integer value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetInt(const std::string& id, int value);
  /*!
   \brief Sets the real number value of the setting with the given identifier.

   \param id Setting identifier
   \param value Real number value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetNumber(const std::string& id, double value);
  /*!
   \brief Sets the string value of the setting with the given identifier.

   \param id Setting identifier
   \param value String value to set
   \return True if setting the value was successful, false otherwise
   */
  bool SetString(const std::string& id, const std::string& value);
  /*!
   \brief Sets the values of the list setting with the given identifier.

   \param id Setting identifier
   \param value Values to set
   \return True if setting the values was successful, false otherwise
   */
  bool SetList(const std::string& id, const std::vector<CVariant>& value);

  /*!
  \brief Sets the value of the setting with the given identifier to its default.

  \param id Setting identifier
  \return True if setting the value to its default was successful, false otherwise
  */
  bool SetDefault(const std::string &id);
  /*!
  \brief Sets the value of all settings to their default.
  */
  void SetDefaults();

protected:
  CSettingsBase();

  virtual void InitializeSettingTypes() { }
  virtual void InitializeControls() { }
  virtual void InitializeOptionFillers() { }
  virtual void UninitializeOptionFillers() { }
  virtual void InitializeConditions() { }
  virtual bool InitializeDefinitions() = 0;
  virtual void InitializeVisibility() { }
  virtual void InitializeDefaults() { }
  virtual void InitializeISettingsHandlers() { }
  virtual void UninitializeISettingsHandlers() { }
  virtual void InitializeISubSettings() { }
  virtual void UninitializeISubSettings() { }
  virtual void InitializeISettingCallbacks() { }
  virtual void UninitializeISettingCallbacks() { }

  bool InitializeDefinitionsFromXml(const CXBMCTinyXML& xml);

  /*!
  \brief Loads setting values from the given document in XML format

  \param xml Document in XML format from which the settings are loaded
  \param updated Output parameter indicating whether setting values had to be updated
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool LoadValuesFromXml(const CXBMCTinyXML& xml, bool& updated);
  /*!
  \brief Saves the setting values in XML format to the given document.

  \param xml Document to save the setting values in XML format into
  \return True if the setting values were successfully saved, false otherwise
  */
  bool SaveValuesToXml(CXBMCTinyXML& xml) const;

  /*!
  \brief Loads setting values from the given XML element.

  \param root XML element containing setting values
  \param updated Output parameter indicating whether setting values had to be updated
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool LoadValuesFromXml(const TiXmlElement* root, bool& updated);

  /*!
  \brief Loads hidden setting values from the given XML element.

  \param root XML element containing setting values
  \return True if the setting values were successfully loaded, false otherwise
  */
  bool LoadHiddenValuesFromXml(const TiXmlElement* root);

  bool m_initialized;
  CSettingsManager* m_settingsManager;
  CCriticalSection m_critical;
};
