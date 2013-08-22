#ifndef _PLAYER_H_
#define _PLAYER_H_


#include <codec.h>
#include <player_type.h>
#include <player_error.h>
#include <message.h>


#ifdef  __cplusplus
extern "C" {
#endif
 
int 	player_init();
int     player_start(play_control_t *p,unsigned long  priv);
int 	player_stop(int pid);
int 	player_stop_async(int pid);
int     player_exit(int pid);
int 	player_pause(int pid);
int	 	player_resume(int pid);
int 	player_timesearch(int pid,float s_time);
int     player_forward(int pid,int speed);
int     player_backward(int pid,int speed);
int     player_aid(int pid,int audio_id);
int     player_sid(int pid,int sub_id);
int 	player_progress_exit(void);
int     player_list_allpid(pid_info_t *pid);
int     check_pid_valid(int pid);
int 	player_get_play_info(int pid,player_info_t *info);
int 	player_get_media_info(int pid,media_info_t *minfo);
int 	player_video_overlay_en(unsigned enable);
int 	player_start_play(int pid);
int 	player_send_message(int pid, player_cmd_t *cmd);
player_status 	player_get_state(int pid);
unsigned int 	player_get_extern_priv(int pid);
int     player_enable_autobuffer(int pid, int enable);
int     player_set_autobuffer_level(int pid, float min, float middle, float max);

int 	audio_set_mute(int pid,int mute);
int 	audio_get_volume_range(int pid,float *min,float *max);
int 	audio_set_volume(int pid,float val);
int 	audio_get_volume(int pid, float *val);

int 	audio_set_lrvolume(int pid,float lvol,float rvol);
int 	audio_get_lrvolume(int pid, float* lvol,float* rvol);

int 	audio_set_volume_balance(int pid,int balance);
int 	audio_swap_left_right(int pid);
int 	audio_left_mono(int pid);
int 	audio_right_mono(int pid);
int 	audio_stereo(int pid);
int 	audio_set_spectrum_switch(int pid,int isStart,int interval);
int 	player_register_update_callback(callback_t *cb,update_state_fun_t up_fn,int interval_s);
char 	*player_status2str(player_status status);
int 	player_cache_system_init(int enable,const char*dir,int max_size,int block_size);

//control interface
int     player_loop(int pid);
int     player_noloop(int pid);

int 	check_url_type(char *filename);
int 	play_list_player(play_control_t *pctrl,unsigned long priv);

//freescale
int 	enable_freescale(int cfg);
int 	disable_freescale(int cfg);
int   disable_freescale_MBX();
int   enable_2Xscale();
int   enable_2XYscale();
int   enable_freescale_MBX();
int   disable_2X_2XYscale();
int   GL_2X_scale(int mSwitch);

#ifdef  __cplusplus
}
#endif

#endif

