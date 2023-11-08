/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameClientStreamVideo.h"

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CGameClientStreamSwFramebuffer : public CGameClientStreamVideo
{
public:
  CGameClientStreamSwFramebuffer() = default;
  ~CGameClientStreamSwFramebuffer() override = default;

  // Implementation of IGameClientStream via CGameClientStreamVideo
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer) override;
};

} // namespace GAME
} // namespace KODI
