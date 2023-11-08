/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/hardware/IHardwareInput.h"

namespace KODI
{
namespace GAME
{
class CGameClient;

/*!
 * \ingroup games
 *
 * \brief Handles events for hardware such as reset buttons
 */
class CGameClientHardware : public HARDWARE::IHardwareInput
{
public:
  /*!
   * \brief Constructor
   *
   * \param gameClient The game client implementation
   */
  explicit CGameClientHardware(CGameClient& gameClient);

  ~CGameClientHardware() override = default;

  // Implementation of IHardwareInput
  void OnResetButton() override;

private:
  // Construction parameter
  CGameClient& m_gameClient;
};
} // namespace GAME
} // namespace KODI
