/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

namespace KODI
{
namespace JOYSTICK
{
  class IInputHandler;

  /*!
   * \ingroup joystick
   * \brief Interface for classes that can provide input
   */
  class IInputProvider
  {
  public:
    virtual ~IInputProvider() = default;

    /*!
     * \brief Register a handler for the provided input
     *
     * \param handler The handler to receive input provided by this class
     * \param bPromiscuous  If true, receives all input (including handled input)
     *                      in the background
     */
    virtual void RegisterInputHandler(IInputHandler* handler, bool bPromiscuous) = 0;

    /*!
     * \brief Unregister a handler
     *
     * \param handler The handler that was receiving input
     */
    virtual void UnregisterInputHandler(IInputHandler* handler) = 0;
  };
}
}
