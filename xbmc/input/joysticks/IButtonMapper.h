/*
 *      Copyright (C) 2014-2016 Team Kodi
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

namespace JOYSTICK
{
  class CDriverPrimitive;
  class IButtonMap;
  class IButtonMapCallback;

  /*!
   * \ingroup joysticks
   *
   * \brief Button mapper interface to assign the driver's raw button/hat/axis
   *        elements to physical joystick features using a provided button map.
   *
   * \sa IButtonMap
   */
  class IButtonMapper
  {
  public:
    IButtonMapper() : m_callback(nullptr) { }

    virtual ~IButtonMapper(void) { }

    /*!
     * \brief The add-on ID of the game controller associated with this button mapper
     *
     * \return The ID of the add-on extending kodi.game.controller
     */
    virtual std::string ControllerID(void) const = 0;

    /*!
     * \brief Handle button/hat press or axis threshold
     *
     * \param buttonMap  The button map being manipulated
     * \param primitive  The source of the action
     *
     * \return True if action was mapped to a feature
     */
    virtual bool MapPrimitive(IButtonMap* buttonMap, const CDriverPrimitive& primitive) = 0;

    /*!
     * \brief Get the duration for which this mapper has been accepting input
     *        for mapping buttons
     *
     * \return The duration, in ms, or -1 if this mapper is not mapping buttons
     */
    virtual int MappingDurationMs(void) const = 0;

    // Button map callback interface
    void SetButtonMapCallback(IButtonMapCallback* callback) { m_callback = callback; }
    void ResetButtonMapCallback(void) { m_callback = nullptr; }
    IButtonMapCallback* ButtonMapCallback(void) { return m_callback; }

  private:
    IButtonMapCallback* m_callback;
  };
}
