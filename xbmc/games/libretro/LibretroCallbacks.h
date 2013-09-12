/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "libretro_wrapped.h"

namespace GAMES
{
  /**
   * Class containing mutual callbacks of ILibretroCallbacksAV and
   * ILibretroCallbacksDLL.
   */
  class ILibretroCallbacksCommon
  {
  public:
    virtual ~ILibretroCallbacksCommon() { }
    virtual void SetPixelFormat(LIBRETRO::retro_pixel_format format) = 0;
    virtual void SetKeyboardCallback(LIBRETRO::retro_keyboard_event_t callback) = 0;
  };

  /**
   * Callbacks for loading a libretro DLL.
   */
  class ILibretroCallbacksDLL : public ILibretroCallbacksCommon
  {
  public:
    virtual ~ILibretroCallbacksDLL() { }
    virtual void SetDiskControlCallback(const LIBRETRO::retro_disk_control_callback *callback_struct) = 0;
    virtual void SetRenderCallback(const LIBRETRO::retro_hw_render_callback *callback_struct) = 0;
  };

  /**
   * Audio, video and input callbacks for running games. Data is passed in and
   * out of running game clients through these callbacks.
   */
  class ILibretroCallbacksAV : public ILibretroCallbacksCommon
  {
  public:
    virtual ~ILibretroCallbacksAV() { }

    // Data receivers
    virtual void VideoFrame(const void *data, unsigned width, unsigned height, size_t pitch) = 0;
    virtual void AudioSample(int16_t left, int16_t right) = 0;
    virtual size_t AudioSampleBatch(const int16_t *data, size_t frames) = 0;

    // Data reporters
    virtual int16_t GetInputState(unsigned port, unsigned device, unsigned index, unsigned id) = 0;
  };
} // namespace GAMES
