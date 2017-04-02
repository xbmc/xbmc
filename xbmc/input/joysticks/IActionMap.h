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

#include "JoystickTypes.h"
#include "input/Key.h"

namespace KODI
{
namespace JOYSTICK
{
  class CDriverPrimitive;

  /*!
   * \brief Interface for translating features to action IDs
   */
  class IActionMap
  {
  public:
    virtual ~IActionMap() = default;

    /*!
     * \brief The add-on ID of the game controller associated with this action map
     *
     * The controller ID provided by the implementation serves as the context
     * for the feature names below.
     *
     * \return The ID of this action map's game controller add-on
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief Get the action ID mapped to the specified feature
     *
     * \param feature  The feature to look up
     *
     * \return The action ID from Key.h, or ACTION_NONE if no action is mapped
     *         to the specified feature
     */
    virtual int GetActionID(const FeatureName& feature) = 0;
  };
}
}
