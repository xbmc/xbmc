/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 *
 * \brief A list populated by input ports on a game console
 *
 * In the Player Viewer dialog (<b>`GameAgents`</b> window), this list has
 * control ID 4.
 *
 * Each port is an item in the list. Ports are represented by the controller
 * icon of the connected controller.
 *
 * Ports are only included in the list if the controller profile provides
 * input. For example, Multitaps will be skipped.
 *
 * When ports are changed, the player list (\ref IAgentList) is updated.
 */
class IActivePortList
{
public:
  virtual ~IActivePortList() = default;

  /*!
   * \brief Initialize resources
   *
   * \param gameClient The game client providing the ports
   *
   * \return True if the resource is initialized and can be used, false if
   *         the resource failed to initialize and must not be used
   */
  virtual bool Initialize(GameClientPtr gameClient) = 0;

  /*!
   * \brief Deinitialize resources
   */
  virtual void Deinitialize() = 0;

  /*!
   * \brief Refresh the contents of the list
   */
  virtual void Refresh() = 0;
};

} // namespace GAME
} // namespace KODI
