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

class CGameAgent;
class CGameClient;
class CGameClientDevice;
class CGameClientPort;

/*!
 * \ingroup games
 *
 * \brief Smart pointer to a game client (\ref CGameClient)
 */
using GameClientPtr = std::shared_ptr<CGameClient>;

/*!
 * \ingroup games
 *
 * \brief Vector of smart pointers to a game client (\ref CGameClient)
 */
using GameClientVector = std::vector<GameClientPtr>;

/*!
 * \ingroup games
 *
 * \brief Smart pointer to an input port for a game client (\ref CGameClientPort)
 */
using GameClientPortPtr = std::unique_ptr<CGameClientPort>;

/*!
 * \ingroup games
 *
 * \brief Vector of smart pointers to input ports for a game client (\ref CGameClientPort)
 */
using GameClientPortVec = std::vector<GameClientPortPtr>;

/*!
 * \ingroup games
 *
 * \brief Smart pointer to an input device for a game client (\ref CGameClientDevice)
 */
using GameClientDevicePtr = std::unique_ptr<CGameClientDevice>;

/*!
 * \ingroup games
 *
 * \brief Vector of smart pointers to input devices for a game client (\ref CGameClientDevice)
 */
using GameClientDeviceVec = std::vector<GameClientDevicePtr>;

/*!
 * \ingroup games
 *
 * \brief Smart pointer to a game-playing agent (\ref CGameAgent)
 */
using GameAgentPtr = std::shared_ptr<CGameAgent>;

/*!
 * \ingroup games
 *
 * \brief Vector of smart pointers to game-playing agents (\ref CGameAgent)
 */
using GameAgentVec = std::vector<GameAgentPtr>;

/*!
 * \ingroup games
 *
 * \brief Name of the resources directory for game clients
 */
constexpr auto GAME_CLIENT_RESOURCES_DIRECTORY = "resources";
} // namespace GAME
} // namespace KODI
