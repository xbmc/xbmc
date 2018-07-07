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

#include "GameClientStreamSwFramebuffer.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "cores/RetroPlayer/streams/RetroPlayerVideo.h"
#include "games/addons/GameClientTranslator.h"

using namespace KODI;
using namespace GAME;

bool CGameClientStreamSwFramebuffer::GetBuffer(unsigned int width, unsigned int height, game_stream_buffer &buffer)
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
