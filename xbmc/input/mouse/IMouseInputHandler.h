/*
 *      Copyright (C) 2016 Team Kodi
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
   * \brief Interface for handling mouse events
   */
  class IMouseInputHandler
  {
  public:
    virtual ~IMouseInputHandler(void) = default;

    /*!
     * \brief The controller profile for this mouse input handler
     *
     * \return The ID of the add-on extending kodi.game.controller
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief A relative pointer has moved
     *
     * \param relpointer   The name of the relative pointer being moved
     * \param dx           The relative x coordinate of motion
     * \param dy           The relative y coordinate of motion
     *
     * \return True if the event was handled, otherwise false
     */
    virtual bool OnMotion(const std::string& relpointer, int dx, int dy) = 0;

    /*!
     * \brief A mouse button has been pressed
     *
     * \param button      The name of the feature being pressed
     *
     * \return True if the event was handled, otherwise false
     */
    virtual bool OnButtonPress(const std::string& button) = 0;

    /*!
     * \brief A mouse button has been released
     *
     * \param button      The name of the feature being released
     */
    virtual void OnButtonRelease(const std::string& button) = 0;
  };
}
}
