/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientSubsystem.h"

#include "GameClient.h"
#include "GameClientProperties.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/addons/cheevos/GameClientCheevos.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/streams/GameClientStreams.h"

#include <memory>

using namespace KODI;
using namespace GAME;

CGameClientSubsystem::CGameClientSubsystem(CGameClient& gameClient,
                                           AddonInstance_Game& addonStruct,
                                           CCriticalSection& clientAccess)
  : m_gameClient(gameClient), m_struct(addonStruct), m_clientAccess(clientAccess)
{
}

CGameClientSubsystem::~CGameClientSubsystem() = default;

GameClientSubsystems CGameClientSubsystem::CreateSubsystems(CGameClient& gameClient,
                                                            AddonInstance_Game& gameStruct,
                                                            CCriticalSection& clientAccess)
{
  GameClientSubsystems subsystems = {};

  subsystems.Cheevos = std::make_unique<CGameClientCheevos>(gameClient, gameStruct);
  subsystems.Input = std::make_unique<CGameClientInput>(gameClient, gameStruct, clientAccess);
  subsystems.AddonProperties =
      std::make_unique<CGameClientProperties>(gameClient, *gameStruct.props);
  subsystems.Streams = std::make_unique<CGameClientStreams>(gameClient);

  return subsystems;
}

void CGameClientSubsystem::DestroySubsystems(GameClientSubsystems& subsystems)
{
  subsystems.Cheevos.reset();
  subsystems.Input.reset();
  subsystems.AddonProperties.reset();
  subsystems.Streams.reset();
}

CGameClientCheevos& CGameClientSubsystem::Cheevos() const
{
  return m_gameClient.Cheevos();
}

CGameClientInput& CGameClientSubsystem::Input() const
{
  return m_gameClient.Input();
}

CGameClientProperties& CGameClientSubsystem::AddonProperties() const
{
  return m_gameClient.AddonProperties();
}

CGameClientStreams& CGameClientSubsystem::Streams() const
{
  return m_gameClient.Streams();
}
