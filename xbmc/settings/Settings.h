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

#include <set>
#include <string>
#include <vector>

#include <memory>

#include "settings/SettingControl.h"
#include "settings/SettingCreator.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Variant.h"

class CSetting;
class CSettingList;
class CSettingSection;
class CSettingsManager;
class TiXmlElement;
class TiXmlNode;

/*!
 \brief Wrapper around CSettingsManager responsible for properly setting up
 the settings manager and registering all the callbacks, handlers and custom
 setting types.
 \sa CSettingsManager
 */
class CSettings : public CSettingCreator, public CSettingControlCreator
{
public:
  /*!
   \brief Creates a new settings wrapper around a new settings manager.

   For access to the "global" settings wrapper the static Get() method should
   be used.
   */
  CSettings();
  virtual ~CSettings();

  /*!
   \brief Returns a "global" settings wrapper which can be used from anywhere.

   \return "global" settings wrapper
   */
  static CSettings& Get();

  CSettingsManager* GetSettingsManager() const { return m_settingsManager; }

  /*!
   \brief Initializes the setting system with the generic
   settings definition and platform specific setting definitions.

   \return True if the initialization was successful, false otherwise
   */
  bool Initialize();
  /*!
   \brief Loads the setting values.

   \return True if the setting values are successfully loaded, false otherwise
   */
  bool Load();
  /*!
   \brief Loads setting values from the given (XML) file.

   \param file Path to an XML file containing setting values
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const std::string &file);
  /*!
   \brief Loads setting values from the given XML element.

   \param root XML element containing setting values
   \param hide Whether to hide the loaded settings or not
   \return True if the setting values were successfully loaded, false otherwise
   */
  bool Load(const TiXmlElement *root, bool hide = false);
  /*!
   \brief Tells the settings system that all setting values
   have been loaded.

   This manual trigger is necessary to enable the ISettingCallback methods
   being executed.
   */
  void SetLoaded();
  /*!
   \brief Saves the setting values.

   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save();
  /*!
   \brief Saves the setting values to the given (XML) file.

   \param file Path to an XML file
   \return True if the setting values were successfully saved, false otherwise
   */
  bool Save(const std::string &file);
  /*!
   \brief Unloads the previously loaded setting values.

   The values of all the settings are reset to their default values.
   */
  void Unload();
  /*!
   \brief Uninitializes the settings system.

   Unregisters all previously registered callbacks and destroys all setting
   objects.
   */
  void Uninitialize();

  /*!
   \brief Registers the given ISettingCallback implementation for the given
   set of settings.

   \param callback ISettingCallback implementation
   \param settingList List of setting identifiers for which the given callback shall be triggered
   */
  void RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList);
  /*!
   \brief Unregisters the given ISettingCallback implementation.

   \param callback ISettingCallback implementation
   */
  void UnregisterCallback(ISettingCallback *callback);

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
  std::vector<CVariant> GetList(const std::string &id) const;

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
  bool SetList(const std::string &id, const std::vector<CVariant> &value);

  /*!
   \brief Loads the setting being represented by the given XML node with the
   given identifier.

   \param node XML node representing the setting to load
   \param settingId Setting identifier
   \return True if the setting was successfully loaded from the given XML node, false otherwise
   */
  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);
private:
  CSettings(const CSettings&);
  CSettings const& operator=(CSettings const&);

  bool Initialize(const std::string &file);
  bool InitializeDefinitions();
  void InitializeSettingTypes();
  void InitializeControls();
  void InitializeVisibility();
  void InitializeDefaults();
  void InitializeOptionFillers();
  void InitializeConditions();
  void InitializeISettingsHandlers();
  void InitializeISubSettings();
  void InitializeISettingCallbacks();
  bool Reset();

  bool m_initialized;
  CSettingsManager *m_settingsManager;
  CCriticalSection m_critical;
};
