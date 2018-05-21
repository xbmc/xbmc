/*
 *      Copyright (C) 2015-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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
