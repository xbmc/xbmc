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

#include "GameClientSubsystem.h"
#include "GameClient.h"
#include "GameClientProperties.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/addons/input/GameClientInput.h"

using namespace KODI;
using namespace GAME;

CGameClientSubsystem::CGameClientSubsystem(CGameClient &gameClient,
                                           AddonInstance_Game &addonStruct,
                                           CCriticalSection &clientAccess) :
  m_gameClient(gameClient),
  m_struct(addonStruct),
  m_clientAccess(clientAccess)
{
}

CGameClientSubsystem::~CGameClientSubsystem() = default;

GameClientSubsystems CGameClientSubsystem::CreateSubsystems(CGameClient &gameClient, AddonInstance_Game &gameStruct, CCriticalSection &clientAccess)
{
  GameClientSubsystems subsystems = {};

  subsystems.Input.reset(new CGameClientInput(gameClient, gameStruct, clientAccess));
  subsystems.AddonProperties.reset(new CGameClientProperties(gameClient, gameStruct.props));

  return subsystems;
}

void CGameClientSubsystem::DestroySubsystems(GameClientSubsystems &subsystems)
{
  subsystems.Input.reset();
  subsystems.AddonProperties.reset();
}

CGameClientInput &CGameClientSubsystem::Input() const
{
  return m_gameClient.Input();
}

CGameClientProperties &CGameClientSubsystem::AddonProperties() const
{
  return m_gameClient.AddonProperties();
}
