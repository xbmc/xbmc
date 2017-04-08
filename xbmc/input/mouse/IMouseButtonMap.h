/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>

namespace KODI
{
namespace MOUSE
{
  /*!
   * \ingroup mouse
   * \brief Button map interface to translate between the mouse's driver data
   *        and its higher-level features.
   */
  class IMouseButtonMap
  {
  public:
    virtual ~IMouseButtonMap(void) = default;

    /*!
     * \brief The ID of the controller profile associated with this button map
     *
     * The controller ID provided by the implementation serves as the context
     * for the feature names below.
     *
     * \return The ID of this button map's controller profile
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief Get the name of a button by its index
     *
     * \param      buttonIndex   The index of the button
     * \param[out] feature       The name of the feature with the specified button index
     *
     * \return True if the button index is associated with a feature, false otherwise
     */
    virtual bool GetButton(unsigned int buttonIndex, std::string& feature) = 0;

    /*!
     * \brief Get the name of the mouse's relative pointer
     *
     * \param[out] feature       The name of the relative pointer
     *
     * \return True if the mouse has a relative pointer, false otherwise
     */
    virtual bool GetRelativePointer(std::string& feature) = 0;

    /*!
     * \brief Get the button index for a button
     *
     * \param feature        The name of the button
     * \param buttonIndex    The resolved button index
     *
     * \return True if the feature resolved to a button index, or false otherwise
     */
    virtual bool GetButtonIndex(const std::string& feature, unsigned int& buttonIndex) = 0;
  };
}
}
