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

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \brief Interface for handling button maps
   */
  class IButtonMapCallback
  {
  public:
    virtual ~IButtonMapCallback() = default;

    /*!
     * \brief Save the button map
     */
    virtual void SaveButtonMap() = 0;

    /*!
     * \brief Clear the list of ignored driver primitives
     *
     * Called if the user begins capturing primitives to be ignored, and
     * no primitives are captured before the dialog is accepted by the user.
     *
     * In this case, the button mapper won't have been given access to the
     * button map, so a callback is needed to indicate that no primitives were
     * captured and the user accepted this.
     */
    virtual void ResetIgnoredPrimitives() = 0;

    /*!
     * \brief Revert changes to the button map since the last time it was loaded
     *        or commited to disk
     */
    virtual void RevertButtonMap() = 0;
  };
}
}
