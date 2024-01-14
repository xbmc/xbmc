/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace JOYSTICK
{
class IInputHandler;

/*!
 * \ingroup joystick
 *
 * \brief Interface for classes that can provide input
 */
class IInputProvider
{
public:
  virtual ~IInputProvider() = default;

  /*!
   * \brief Register a handler for the provided input
   *
   * \param handler The handler to receive input provided by this class
   * \param bPromiscuous  If true, receives all input (including handled input)
   *                      in the background
   */
  virtual void RegisterInputHandler(IInputHandler* handler, bool bPromiscuous) = 0;

  /*!
   * \brief Unregister a handler
   *
   * \param handler The handler that was receiving input
   */
  virtual void UnregisterInputHandler(IInputHandler* handler) = 0;
};
} // namespace JOYSTICK
} // namespace KODI
