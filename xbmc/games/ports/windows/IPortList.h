/*
 *  Copyright (C) 2021 Team Kodi
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
 * \brief A list populated by controller ports for the port setup window
 *
 * The port setup window presents a list of ports and their attached
 * controllers.
 *
 * The label2 of each port is the currently-connected controller. The user
 * selects from all controllers that the port accepts (as given by the
 * game-addon's <b>`topology.xml`</b> file).
 *
 * The controller topology is stored as a generic tree. Here we apply game logic
 * to simplify controller selection.
 */
class IPortList
{
public:
  virtual ~IPortList() = default;

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
   * \param gameClient The game client providing the ports
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
  virtual bool HasControl(int controlId) = 0;

  /*!
   * \brief Query the ID of the current control in this list
   *
   * \return The control ID, or -1 if no control is currently active
   */
  virtual int GetCurrentControl() = 0;

  /*!
   * \brief Refresh the contents of the list
   */
  virtual void Refresh() = 0;

  /*!
   * \brief Callback when a frame is rendered by the GUI
   */
  virtual void FrameMove() = 0;

  /*!
   * \brief The port list has been focused in the GUI
   */
  virtual void SetFocused() = 0;

  /*!
   * \brief The port list has been selected
   *
   * \brief True if a control was active, false of all controls were inactive
   */
  virtual bool OnSelect() = 0;

  /*!
   * \brief Reset the ports to their game add-on's default configuration
   */
  virtual void ResetPorts() = 0;
};
} // namespace GAME
} // namespace KODI
