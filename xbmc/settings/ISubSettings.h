/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace tinyxml2
{
class XMLNode;
}

/*!
 \ingroup settings
 \brief Interface defining methods to load additional setting values from an
 XML file being loaded by the settings system.
 */
class ISubSettings
{
public:
  virtual ~ISubSettings() = default;

  /*!
   \brief Load settings from the given XML node.

   \param settings XML node containing setting values
   \return True if loading the settings was successful, false otherwise.
   */
  virtual bool Load(const tinyxml2::XMLNode* settings) { return true; }
  /*!
   \brief Save settings to the given XML node.

   \param settings XML node in which the settings will be saved
   \return True if saving the settings was successful, false otherwise.
   */
  virtual bool Save(tinyxml2::XMLNode* settings) const { return true; }
  /*!
   \brief Clear any loaded setting values.
   */
  virtual void Clear() { }
};
