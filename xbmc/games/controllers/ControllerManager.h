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

#include "ControllerTypes.h"
#include "addons/IAddon.h"

#include <map>
#include <set>
#include <string>

namespace KODI
{
namespace GAME
{
  class CControllerManager
  {
  public:
    CControllerManager() = default;
    ~CControllerManager() = default;

    /*!
     * \brief Get a controller
     *
     * A cache is used to avoid reloading controllers each time they are
     * requested.
     *
     * \param controllerId The controller's ID
     *
     * \return The controller, or empty if the controller isn't installed or
     *         can't be loaded
     */
    ControllerPtr GetController(const std::string& controllerId);

    /*!
     * \brief Get the default controller
     *
     * \return The default controller, or empty if the controller failed to load
     */
    ControllerPtr GetDefaultController();

    /*!
     * \brief Get installed controllers
     *
     * \return The installed controllers that loaded successfully
     */
    ControllerVector GetControllers();

  private:
    ControllerPtr LoadController(ADDON::AddonPtr addon);

    std::map<std::string, ControllerPtr> m_cache;
    std::set<std::string> m_failedControllers; // Controllers that failed to load
  };
}
}
