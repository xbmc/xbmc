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

  class CGameClient;
  using GameClientPtr = std::shared_ptr<CGameClient>;
  using GameClientVector = std::vector<GameClientPtr>;

  class CGameClientPort;
  using GameClientPortPtr = std::unique_ptr<CGameClientPort>;
  using GameClientPortVec = std::vector<GameClientPortPtr>;

  class CGameClientDevice;
  using GameClientDevicePtr = std::unique_ptr<CGameClientDevice>;
  using GameClientDeviceVec = std::vector<GameClientDevicePtr>;

}
}
