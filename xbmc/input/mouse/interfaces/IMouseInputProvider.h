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
namespace MOUSE
{
class IMouseInputHandler;

/*!
 * \ingroup mouse
 *
 * \brief Interface for classes that can provide mouse input
 */
class IMouseInputProvider
{
public:
  virtual ~IMouseInputProvider() = default;

  /*!
   * \brief Registers a handler to be called on mouse input
   *
   * \param handler The handler to receive mouse input provided by this class
   * \param bPromiscuous True to observe all events without affecting
   *        subsequent handlers
   * \param forceDefaultMap Always use the default keyboard buttonmap, avoiding
   *        buttonmaps provided by add-ons
   */
  virtual void RegisterMouseHandler(IMouseInputHandler* handler,
                                    bool bPromiscuous,
                                    bool forceDefaultMap) = 0;

  /*!
   * \brief Unregisters handler from mouse input
   *
   * \param handler The handler that was receiving mouse input
   */
  virtual void UnregisterMouseHandler(IMouseInputHandler* handler) = 0;
};
} // namespace MOUSE
} // namespace KODI
