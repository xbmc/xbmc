/*
 *  Copyright (C) 2021-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief A list populated by the controllers of game-playing agents (\ref CGameAgent)
 *
 * This class manages the behavior of the controller list (with control ID 7) in
 * the player dialog (<b>`GameAgents`</b> window).
 *
 * The active ports are determined by \ref IActivePortList.
 *
 * The list is populated by the \ref CGUIGameControllerProvider.
 */
class IAgentControllerList
{
public:
  virtual ~IAgentControllerList() = default;

  /*!
   * \brief Callback when the GUI window is loaded
   */
  virtual void OnWindowLoaded() = 0;

  /*!
   * \brief Callback when the GUI window is unloaded
   */
  virtual void OnWindowUnload() = 0;

  /*!
   * \brief Initialize resources
   *
   * \param gameClient The active game client, an empty pointer if no game
   *        client is active
   *
   * \return True if the resource is initialized and can be used, false if the
   *         resource failed to initialize and must not be used
   */
  virtual bool Initialize(GameClientPtr gameClient) = 0;

  /*!
   * \brief Deinitialize resources
   */
  virtual void Deinitialize() = 0;

  /*!
   * \brief Query if a control with the given ID belongs to this list
   */
  virtual bool HasControl(int controlId) const = 0;

  /*!
   * \brief Query the ID of the current control in this list
   */
  virtual int GetCurrentControl() const = 0;

  /*!
   * \brief Called once per frame
   *
   * This allows the list to update its currently focused item.
   */
  virtual void FrameMove() = 0;

  /*!
   * \brief Refresh the contents of the list
   */
  virtual void Refresh() = 0;

  /*!
   * \brief The agent list has been focused in the GUI
   */
  virtual void SetFocused() = 0;

  /*!
   * \brief The agent list has been selected in the GUI
   */
  virtual void OnSelect() = 0;
};
} // namespace GAME
} // namespace KODI
