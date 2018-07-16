/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
     * \brief Get the default keyboard
     *
     * \return The keyboard controller, or empty if the controller failed to load
     */
    ControllerPtr GetDefaultKeyboard();

    /*!
     * \brief Get the default mouse
     *
     * \return The mouse controller, or empty if the controller failed to load
     */
    ControllerPtr GetDefaultMouse();

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
