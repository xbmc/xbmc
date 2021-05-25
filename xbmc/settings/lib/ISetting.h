/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SettingRequirement.h"

#include <string>

class CSettingsManager;
class TiXmlNode;

/*!
 \ingroup settings
 \brief Interface defining the base of all setting objects
 */
class ISetting
{
public:
  /*!
   \brief Creates a new setting object with the given identifier.

   \param id Identifier of the setting object
   \param settingsManager Reference to the settings manager
   */
  ISetting(const std::string &id, CSettingsManager *settingsManager = nullptr);
  virtual ~ISetting() = default;

  /*!
   \brief Deserializes the given XML node into the properties of the setting
   object.

   If the update parameter is true, the checks for mandatory properties are
   skipped and values are only updated.

   \param node XML node containing the properties of the setting object
   \param update Whether to perform checks for mandatory properties or not
   \return True if deserialization was successful, false otherwise
   */
  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  /*!
   \brief Gets the identifier of the setting object.

   \return Identifier of the setting object
   */
  const std::string& GetId() const { return m_id; }
  /*!
   \brief Whether the setting object is visible or hidden.

   \return True if the setting object is visible, false otherwise
   */
  virtual bool IsVisible() const { return m_visible; }
  /*!
   \brief Sets the visibility state of the setting object.

   \param visible Whether the setting object shall be visible or not
   */
  virtual void SetVisible(bool visible) { m_visible = visible; }
   /*!
   \brief Gets the localizeable label ID of the setting group.

   \return Localizeable label ID of the setting group
   */
  int GetLabel() const { return m_label; }
  /*!
   \brief Sets the localizeable label ID of the setting group.

   \param label Localizeable label ID of the setting group
   */
  void SetLabel(int label) { m_label = label; }
  /*!
   \brief Gets the localizeable help ID of the setting group.

   \return Localizeable help ID of the setting group
   */
  int GetHelp() const { return m_help; }
  /*!
   \brief Sets the localizeable help ID of the setting group.

   \param label Localizeable help ID of the setting group
   */
  void SetHelp(int help) { m_help = help; }
  /*!
   \brief Whether the setting object meets all necessary requirements.

   \return True if the setting object meets all necessary requirements, false otherwise
   */
  virtual bool MeetsRequirements() const { return m_meetsRequirements; }
  /*!
   \brief Checks if the setting object meets all necessary requirements.
   */
  virtual void CheckRequirements();
  /*!
   \brief Sets whether the setting object meets all necessary requirements.

   \param visible Whether the setting object meets all necessary requirements or not
   */
  virtual void SetRequirementsMet(bool requirementsMet) { m_meetsRequirements = requirementsMet; }

  /*!
   \brief Deserializes the given XML node to retrieve a setting object's
   identifier.

   \param node XML node containing a setting object's identifier
   \param identification Will contain the deserialized setting object's identifier
   \return True if a setting object's identifier was deserialized, false otherwise
   */
  static bool DeserializeIdentification(const TiXmlNode *node, std::string &identification);

protected:
  static constexpr int DefaultLabel = -1;
  /*!
   \brief Deserializes the given XML node to retrieve a setting object's identifier from the given attribute.

   \param node XML node containing a setting object's identifier
   \param attribute Attribute which contains the setting object's identifier
   \param identification Will contain the deserialized setting object's identifier
   \return True if a setting object's identifier was deserialized, false otherwise
   */
  static bool DeserializeIdentificationFromAttribute(const TiXmlNode* node,
                                                     const std::string& attribute,
                                                     std::string& identification);

  std::string m_id;
  CSettingsManager *m_settingsManager;

private:
  bool m_visible = true;
  int m_label = DefaultLabel;
  int m_help = -1;
  bool m_meetsRequirements = true;
  CSettingRequirement m_requirementCondition;
};
