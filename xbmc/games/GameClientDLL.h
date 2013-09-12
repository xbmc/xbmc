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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "games/libretro/libretro_wrapped.h"
#include "DynamicDll.h"

class GameClientDLLInterface
{
public:
    virtual ~GameClientDLLInterface() {}
    virtual void retro_set_environment(LIBRETRO::retro_environment_t env_func) = 0;
    virtual void retro_set_video_refresh(LIBRETRO::retro_video_refresh_t frame_func) = 0;
    virtual void retro_set_audio_sample(LIBRETRO::retro_audio_sample_t sample_func) = 0;
    virtual void retro_set_audio_sample_batch(LIBRETRO::retro_audio_sample_batch_t samples_func) = 0;
    virtual void retro_set_input_poll(LIBRETRO::retro_input_poll_t poll_func) = 0;
    virtual void retro_set_input_state(LIBRETRO::retro_input_state_t state_func) = 0;

    virtual void retro_init(void) = 0;
    virtual void retro_deinit(void) = 0;
    virtual void retro_reset(void) = 0;
    virtual void retro_run(void) = 0;

    virtual unsigned int retro_api_version(void) = 0;
    virtual void retro_get_system_info(LIBRETRO::retro_system_info *info) = 0;
    virtual void retro_get_system_av_info(LIBRETRO::retro_system_av_info *info) = 0;
    virtual void retro_set_controller_port_device(unsigned int port, unsigned int device) = 0;

    virtual size_t retro_serialize_size(void) = 0;
    virtual bool retro_serialize(void *data, size_t size) = 0;
    virtual bool retro_unserialize(const void *data, size_t size) = 0;

    virtual void retro_cheat_reset(void) = 0;
    virtual void retro_cheat_set(unsigned int index, bool enabled, const char *code) = 0;

    virtual bool retro_load_game(const LIBRETRO::retro_game_info *game) = 0;
    virtual bool retro_load_game_special(unsigned int game_type, const LIBRETRO::retro_game_info *info, size_t num_info) = 0;
    virtual void retro_unload_game(void) = 0;

    virtual unsigned int retro_get_region(void) = 0;

    virtual void *retro_get_memory_data(unsigned int id) = 0;
    virtual size_t retro_get_memory_size(unsigned int id) = 0;
};

class GameClientDLL : public DllDynamic, GameClientDLLInterface
{
  DECLARE_DLL_WRAPPER(GameClientDLL, /* Call SetFile() later */ )
  DEFINE_METHOD1(void, retro_set_environment, (LIBRETRO::retro_environment_t p1))
  DEFINE_METHOD1(void, retro_set_video_refresh, (LIBRETRO::retro_video_refresh_t p1))
  DEFINE_METHOD1(void, retro_set_audio_sample, (LIBRETRO::retro_audio_sample_t p1))
  DEFINE_METHOD1(void, retro_set_audio_sample_batch, (LIBRETRO::retro_audio_sample_batch_t p1))
  DEFINE_METHOD1(void, retro_set_input_poll, (LIBRETRO::retro_input_poll_t p1))
  DEFINE_METHOD1(void, retro_set_input_state, (LIBRETRO::retro_input_state_t p1))

  DEFINE_METHOD0(void, retro_init)
  DEFINE_METHOD0(void, retro_deinit)
  DEFINE_METHOD0(void, retro_reset)
  DEFINE_METHOD0(void, retro_run)

  DEFINE_METHOD0(unsigned int, retro_api_version)
  DEFINE_METHOD1(void, retro_get_system_info, (LIBRETRO::retro_system_info *p1))
  DEFINE_METHOD1(void, retro_get_system_av_info, (LIBRETRO::retro_system_av_info *p1))
  DEFINE_METHOD2(void, retro_set_controller_port_device, (unsigned int p1, unsigned int p2))

  DEFINE_METHOD0(size_t, retro_serialize_size)
  DEFINE_METHOD2(bool, retro_serialize, (void *p1, size_t p2))
  DEFINE_METHOD2(bool, retro_unserialize, (const void *p1, size_t p2))

  DEFINE_METHOD0(void, retro_cheat_reset)
  DEFINE_METHOD3(void, retro_cheat_set, (unsigned int p1, bool p2, const char *p3))

  DEFINE_METHOD1(bool, retro_load_game, (const LIBRETRO::retro_game_info *p1))
  DEFINE_METHOD3(bool, retro_load_game_special, (unsigned int p1, const LIBRETRO::retro_game_info *p2, size_t p3))
  DEFINE_METHOD0(void, retro_unload_game)

  DEFINE_METHOD0(unsigned int, retro_get_region)

  DEFINE_METHOD1(void*, retro_get_memory_data, (unsigned int p1))
  DEFINE_METHOD1(size_t, retro_get_memory_size, (unsigned int p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(retro_set_environment)
    RESOLVE_METHOD(retro_set_video_refresh)
    RESOLVE_METHOD(retro_set_audio_sample)
    RESOLVE_METHOD(retro_set_audio_sample_batch)
    RESOLVE_METHOD(retro_set_input_poll)
    RESOLVE_METHOD(retro_set_input_state)

    RESOLVE_METHOD(retro_init)
    RESOLVE_METHOD(retro_deinit)
    RESOLVE_METHOD(retro_reset)
    RESOLVE_METHOD(retro_run)

    RESOLVE_METHOD(retro_api_version)
    RESOLVE_METHOD(retro_get_system_info)
    RESOLVE_METHOD(retro_get_system_av_info)
    RESOLVE_METHOD(retro_set_controller_port_device)

    RESOLVE_METHOD(retro_serialize_size)
    RESOLVE_METHOD(retro_serialize)
    RESOLVE_METHOD(retro_unserialize)

    RESOLVE_METHOD(retro_cheat_reset)
    RESOLVE_METHOD(retro_cheat_set)

    RESOLVE_METHOD(retro_load_game)
    RESOLVE_METHOD(retro_load_game_special)
    RESOLVE_METHOD(retro_unload_game)

    RESOLVE_METHOD(retro_get_region)

    RESOLVE_METHOD(retro_get_memory_data)
    RESOLVE_METHOD(retro_get_memory_size)

  END_METHOD_RESOLVE()
};
