/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientStreamSwFramebuffer.h"

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "games/addons/GameClientTranslator.h"

using namespace KODI;
using namespace GAME;

bool CGameClientStreamSwFramebuffer::GetBuffer(unsigned int width,
                                               unsigned int height,
                                               game_stream_buffer& buffer)
{
  if (m_stream != nullptr)
  {
    RETRO::VideoStreamBuffer streamBuffer;
    if (m_stream->GetStreamBuffer(width, height, static_cast<RETRO::StreamBuffer&>(streamBuffer)))
    {
      buffer.type = GAME_STREAM_SW_FRAMEBUFFER;

      game_stream_sw_framebuffer_buffer& framebuffer = buffer.sw_framebuffer;

      framebuffer.format = CGameClientTranslator::TranslatePixelFormat(streamBuffer.pixfmt);
      framebuffer.data = streamBuffer.data;
      framebuffer.size = streamBuffer.size;

      return true;
    }
  }

  return false;
}
