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

class CKey;

namespace KEYBOARD
{
  /*!
   * \brief Interface for translating keys to action IDs
   */
  class IActionMap
  {
  public:
    virtual ~IActionMap() = default;

    /*!
     * \brief Get the action ID mapped to the specified key
     *
     * \param key The key to look up
     *
     * \return The action ID from Action.h, or ACTION_NONE if no action is
     *         mapped to the specified key
     */
    virtual unsigned int GetActionID(const CKey& key) = 0;
  };
}
