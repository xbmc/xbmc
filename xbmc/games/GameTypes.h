/*
 *      Copyright (C) 2015-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
