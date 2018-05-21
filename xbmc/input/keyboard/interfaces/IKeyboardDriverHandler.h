/*
 *      Copyright (C) 2015-present Team Kodi
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

class CKey;

namespace KODI
{
namespace KEYBOARD
{
  /*!
   * \ingroup keyboard
   * \brief Interface for handling keyboard events
   */
  class IKeyboardDriverHandler
  {
  public:
    virtual ~IKeyboardDriverHandler() = default;

    /*!
     * \brief A key has been pressed
     *
     * \param key The pressed key
     *
     * \return True if the event was handled, false otherwise
     */
    virtual bool OnKeyPress(const CKey& key) = 0;

    /*!
     * \brief A key has been released
     *
     * \param key The released key
     */
    virtual void OnKeyRelease(const CKey& key) = 0;
  };
}
}
