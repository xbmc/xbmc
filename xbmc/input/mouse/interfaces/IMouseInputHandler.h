/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/mouse/MouseTypes.h"

#include <string>

namespace KODI
{
namespace MOUSE
{
/*!
 * \ingroup mouse
 *
 * \brief Interface for handling mouse events
 */
class IMouseInputHandler
{
public:
  virtual ~IMouseInputHandler(void) = default;

  /*!
   * \brief The controller profile for this mouse input handler
   *
   * \return The ID of the add-on extending kodi.game.controller
   */
  virtual std::string ControllerID(void) const = 0;

  /*!
   * \brief A relative pointer has moved
   *
   * \param relpointer   The name of the relative pointer being moved
   * \param dx           The relative x coordinate of motion
   * \param dy           The relative y coordinate of motion
   *
   * The mouse uses a left-handed (graphics) cartesian coordinate system.
   * Positive X is right, positive Y is down.
   *
   * \return True if the event was handled, otherwise false
   */
  virtual bool OnMotion(const PointerName& relpointer, int dx, int dy) = 0;

  /*!
   * \brief A mouse button has been pressed
   *
   * \param button      The name of the feature being pressed
   *
   * \return True if the event was handled, otherwise false
   */
  virtual bool OnButtonPress(const ButtonName& button) = 0;

  /*!
   * \brief A mouse button has been released
   *
   * \param button      The name of the feature being released
   */
  virtual void OnButtonRelease(const ButtonName& button) = 0;

  /*!
   * \brief Called at the end of the frame that provided input
   *
   * This can be as a result of a pointer update, a button press, or a button
   * release. All three events will result in a call to OnInputFrame().
   */
  virtual void OnInputFrame() = 0;
};
} // namespace MOUSE
} // namespace KODI
