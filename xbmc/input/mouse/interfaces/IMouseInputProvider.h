/*
 *      Copyright (C) 2017 Team Kodi
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
namespace MOUSE
{
  class IMouseInputHandler;

  /*!
   * \ingroup mouse
   * \brief Interface for classes that can provide mouse input
   */
  class IMouseInputProvider
  {
  public:
    virtual ~IMouseInputProvider() = default;

    /*!
     * \brief Registers a handler to be called on mouse input
     *
     * \param handler The handler to receive mouse input provided by this class
     * \param bPromiscuous True to observe all events without affecting
     *        subsequent handlers
     */
    virtual void RegisterMouseHandler(IMouseInputHandler* handler, bool bPromiscuous) = 0;

    /*!
     * \brief Unregisters handler from mouse input
     *
     * \param handler The handler that was receiving mouse input
     */
    virtual void UnregisterMouseHandler(IMouseInputHandler* handler) = 0;
  };
}
}
