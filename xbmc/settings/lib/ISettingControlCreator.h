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

#include <memory>
#include <string>

class ISettingControl;

/*!
 \ingroup settings
 \brief Interface for creating a new setting control of a custom setting control type.
 */
class ISettingControlCreator
{
public:
  virtual ~ISettingControlCreator() = default;

  /*!
   \brief Creates a new setting control of the given custom setting control type.

   \param controlType string representation of the setting control type
   \return A new setting control object of the given (custom) setting control type or nullptr if the setting control type is unknown
   */
  virtual std::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const = 0;
};
