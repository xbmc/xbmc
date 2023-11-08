/*
 *      Copyright (C) 2018 Team Kodi
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

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/controllers/ControllerTypes.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CGameClientInput;

/*!
 * \ingroup games
 *
 * \brief A container for the layout of a controller connected to a game
 *        client input port
 */
class CGameClientController
{
public:
  /*!
   * \brief Construct a controller layout
   *
   * \brief controller The controller add-on
   */
  CGameClientController(CGameClientInput& input, ControllerPtr controller);

  /*!
   * \brief Get a controller layout for the Game API
   */
  game_controller_layout TranslateController() const;

private:
  // Construction parameters
  CGameClientInput& m_input;
  const ControllerPtr m_controller;

  // Buffer parameters
  std::string m_controllerId;
  std::vector<char*> m_digitalButtons;
  std::vector<char*> m_analogButtons;
  std::vector<char*> m_analogSticks;
  std::vector<char*> m_accelerometers;
  std::vector<char*> m_keys;
  std::vector<char*> m_relPointers;
  std::vector<char*> m_absPointers;
  std::vector<char*> m_motors;
};
} // namespace GAME
} // namespace KODI
