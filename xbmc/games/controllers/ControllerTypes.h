/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

namespace KODI
{
namespace GAME
{
  class CController;
  using ControllerPtr = std::shared_ptr<CController>;
  using ControllerVector = std::vector<ControllerPtr>;

  /*!
   * \brief Type of input provided by a hardware or controller port
   */
  enum class PORT_TYPE
  {
    UNKNOWN,
    KEYBOARD,
    MOUSE,
    CONTROLLER,
  };
}
}
