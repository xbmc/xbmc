#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include "system.h"

#include "DynamicDll.h"

extern "C"
{
#include <player_type.h>
#include <player_error.h>
}

struct AML_URLProtocol;

class DllLibAmplayerInterface
{
public:
  virtual ~DllLibAmplayerInterface() {};

  virtual int player_init(void)=0;
  virtual int player_start(play_control_t *p, unsigned long priv)=0;
  virtual int player_stop(int pid)=0;
  virtual int player_stop_async(int pid)=0;
  virtual int player_exit(int pid)=0;
  virtual int player_pause(int pid)=0;
  virtual int player_resume(int pid)=0;
  virtual int player_timesearch(int pid, float s_time)=0;
  virtual int player_forward(int pid, int speed)=0;
  virtual int player_backward(int pid, int speed)=0;
  virtual int player_aid(int pid, int audio_id)=0;
  virtual int player_sid(int pid, int sub_id)=0;
  virtual int player_progress_exit(void)=0;
  virtual int player_list_allpid(pid_info_t *pid)=0;
  virtual int check_pid_valid(int pid)=0;
  virtual int player_get_play_info(int pid, player_info_t *info)=0;
  virtual int player_get_media_info(int pid, media_info_t *minfo)=0;
  virtual int player_video_overlay_en(unsigned enable)=0;
  virtual int player_start_play(int pid)=0;
  virtual player_status player_get_state(int pid)=0;
  virtual unsigned int player_get_extern_priv(int pid)=0;
  virtual int player_enable_autobuffer(int pid, int enable)=0;
  virtual int player_set_autobuffer_level(int pid, float min, float middle, float max)=0;
  virtual int player_register_update_callback(callback_t *cb,update_state_fun_t up_fn,int interval_s)=0;
  virtual char* player_status2str(player_status status)=0;

  virtual int audio_set_volume(int pid,float val)=0;
  virtual int audio_set_delay(int pid, int delay)=0;
  
  virtual int codec_open_sub_read(void)=0;
  virtual int codec_close_sub_fd(int sub_fd)=0;
  virtual int codec_get_sub_size_fd(int sub_fd)=0;
  virtual int codec_read_sub_data_fd(int sub_fd, char* buf, unsigned int length)=0;

  virtual int av_register_protocol2(AML_URLProtocol *protocol, int size)=0;
};

class DllLibAmplayer : public DllDynamic, DllLibAmplayerInterface
{
  DECLARE_DLL_WRAPPER(DllLibAmplayer, "libamplayer.so")

  DEFINE_METHOD0(int,            player_init)
  DEFINE_METHOD2(int,            player_start,          (play_control_t *p1, unsigned long p2))
  DEFINE_METHOD1(int,            player_stop,           (int p1))
  DEFINE_METHOD1(int,            player_stop_async,     (int p1))
  DEFINE_METHOD1(int,            player_exit,           (int p1))
  DEFINE_METHOD1(int,            player_pause,          (int p1))
  DEFINE_METHOD1(int,            player_resume,         (int p1))
  DEFINE_METHOD2(int,            player_timesearch,     (int p1, float p2))
  DEFINE_METHOD2(int,            player_forward,        (int p1, int p2))
  DEFINE_METHOD2(int,            player_backward,       (int p1, int p2))
  DEFINE_METHOD2(int,            player_aid,            (int p1, int p2))
  DEFINE_METHOD2(int,            player_sid,            (int p1, int p2))
  DEFINE_METHOD0(int,            player_progress_exit)
  DEFINE_METHOD1(int,            player_list_allpid,    (pid_info_t *p1))
  DEFINE_METHOD1(int,            check_pid_valid,       (int p1))
  DEFINE_METHOD2(int,            player_get_play_info,  (int p1, player_info_t *p2))
  DEFINE_METHOD2(int,            player_get_media_info, (int p1, media_info_t *p2))
  DEFINE_METHOD1(int,            player_video_overlay_en, (unsigned int p1))
  DEFINE_METHOD1(int,            player_start_play,     (int p1))
  DEFINE_METHOD1(player_status,  player_get_state,      (int p1))
  DEFINE_METHOD1(unsigned int,   player_get_extern_priv,(int p1))
  DEFINE_METHOD2(int,            player_enable_autobuffer, (int p1, int p2))
  DEFINE_METHOD4(int,            player_set_autobuffer_level, (int p1, float p2, float p3, float p4))
  DEFINE_METHOD3(int,            player_register_update_callback, (callback_t *p1, update_state_fun_t p2, int p3))
  DEFINE_METHOD1(char*,          player_status2str,     (player_status p1))

  DEFINE_METHOD2(int,            audio_set_volume,      (int p1, float p2))
  DEFINE_METHOD2(int,            audio_set_delay,       (int p1, int p2))

  DEFINE_METHOD0(int,            codec_open_sub_read)
  DEFINE_METHOD1(int,            codec_close_sub_fd,    (int p1))
  DEFINE_METHOD1(int,            codec_get_sub_size_fd, (int p1))
  DEFINE_METHOD3(int,            codec_read_sub_data_fd,(int p1, char *p2, unsigned int p3))

  DEFINE_METHOD2(int,            av_register_protocol2, (AML_URLProtocol *p1, int p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(player_init)
    RESOLVE_METHOD(player_start)
    RESOLVE_METHOD(player_stop)
    RESOLVE_METHOD(player_stop_async)
    RESOLVE_METHOD(player_exit)
    RESOLVE_METHOD(player_pause)
    RESOLVE_METHOD(player_resume)
    RESOLVE_METHOD(player_timesearch)
    RESOLVE_METHOD(player_forward)
    RESOLVE_METHOD(player_backward)
    RESOLVE_METHOD(player_aid)
    RESOLVE_METHOD(player_sid)
    RESOLVE_METHOD(player_progress_exit)
    RESOLVE_METHOD(player_list_allpid)
    RESOLVE_METHOD(check_pid_valid)
    RESOLVE_METHOD(player_get_play_info)
    RESOLVE_METHOD(player_get_media_info)
    RESOLVE_METHOD(player_video_overlay_en)
    RESOLVE_METHOD(player_start_play)
    RESOLVE_METHOD(player_get_state)
    RESOLVE_METHOD(player_get_extern_priv)
    RESOLVE_METHOD(player_enable_autobuffer)
    RESOLVE_METHOD(player_set_autobuffer_level)
    RESOLVE_METHOD(player_register_update_callback)
    RESOLVE_METHOD(player_status2str)

    RESOLVE_METHOD(audio_set_volume)
    RESOLVE_METHOD(audio_set_delay)

    RESOLVE_METHOD(codec_open_sub_read)
    RESOLVE_METHOD(codec_close_sub_fd)
    RESOLVE_METHOD(codec_get_sub_size_fd)
    RESOLVE_METHOD(codec_read_sub_data_fd)

    RESOLVE_METHOD(av_register_protocol2)

  END_METHOD_RESOLVE()
};
