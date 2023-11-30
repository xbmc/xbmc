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
class CController;

/*!
 * \ingroup games
 *
 * \brief Smart pointer to a game controller (\ref CController)
 */
using ControllerPtr = std::shared_ptr<CController>;

/*!
 * \ingroup games
 *
 * \brief Vector of smart pointers to a game controller (\ref CController)
 */
using ControllerVector = std::vector<ControllerPtr>;

/*!
 * \ingroup games
 *
 * \brief Type of input provided by a hardware or controller port
 */
enum class PORT_TYPE
{
  UNKNOWN,
  KEYBOARD,
  MOUSE,
  CONTROLLER,
};
} // namespace GAME
} // namespace KODI
