/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "GameClientStreamVideo.h"

namespace KODI
{
namespace GAME
{

class CGameClientStreamSwFramebuffer : public CGameClientStreamVideo
{
public:
  CGameClientStreamSwFramebuffer() = default;
  ~CGameClientStreamSwFramebuffer() override = default;

  // Implementation of IGameClientStream via CGameClientStreamVideo
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer &buffer) override;
};

} // namespace GAME
} // namespace KODI
