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

class TiXmlNode;

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
  virtual bool Load(const TiXmlNode *settings) { return true; }
  /*!
   \brief Save settings to the given XML node.

   \param settings XML node in which the settings will be saved
   \return True if saving the settings was successful, false otherwise.
   */
  virtual bool Save(TiXmlNode *settings) const { return true; }
  /*!
   \brief Clear any loaded setting values.
   */
  virtual void Clear() { }
};
