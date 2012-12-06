#ifndef _PLAYER_TYPE_H_
#define _PLAYER_TYPE_H_

#include <libavformat/avformat.h>
#include <stream_format.h>

#define MSG_SIZE                    64
#define MAX_VIDEO_STREAMS           8
#define MAX_AUDIO_STREAMS           8
#define MAX_SUB_INTERNAL            8
#define MAX_SUB_EXTERNAL            24
#define MAX_SUB_STREAMS             (MAX_SUB_INTERNAL + MAX_SUB_EXTERNAL)
#define MAX_PLAYER_THREADS          32

#define CALLBACK_INTERVAL			(300)

//#define DEBUG_VARIABLE_DUR

typedef enum
{      
	/******************************
	* 0x1000x: 
	* player do parse file
	* decoder not running
	******************************/
	PLAYER_INITING  	= 0x10001,
	PLAYER_TYPE_REDY  = 0x10002,
	PLAYER_INITOK   	= 0x10003,	

	/******************************
	* 0x2000x: 
	* playback status
	* decoder is running
	******************************/
	PLAYER_RUNNING  	= 0x20001,
	PLAYER_BUFFERING 	= 0x20002,
	PLAYER_PAUSE    	= 0x20003,
	PLAYER_SEARCHING	= 0x20004,
	
	PLAYER_SEARCHOK 	= 0x20005,
	PLAYER_START    	= 0x20006,	
	PLAYER_FF_END   	= 0x20007,
	PLAYER_FB_END   	= 0x20008,

	PLAYER_PLAY_NEXT	= 0x20009,	
	PLAYER_BUFFER_OK	= 0x2000a,	
	PLAYER_FOUND_SUB	= 0x2000b,

	/******************************
	* 0x3000x: 
	* player will exit	
	******************************/
	PLAYER_ERROR		= 0x30001,
	PLAYER_PLAYEND  	= 0x30002,	
	PLAYER_STOPED   	= 0x30003,  
	PLAYER_EXIT   		= 0x30004, 

    /******************************
     * 0x4000x:
     * divx drm
     * decoder will exit or give
     * a message dialog
     * ****************************/
    PLAYER_DIVX_AUTHORERR   =   0x40001,
    PLAYER_DIVX_RENTAL_EXPIRED  =   0x40002,
    PLAYER_DIVX_RENTAL_VIEW =   0x40003,

	
}player_status;


typedef struct
{   
	int index;
    int id;    
    int width;
    int height;
    int aspect_ratio_num;
    int aspect_ratio_den;
    int frame_rate_num;
    int frame_rate_den;
	int bit_rate;
    vformat_t format;
    int duartion;
    unsigned int video_rotation_degree;
}mvideo_info_t;

typedef enum
{
    ACOVER_NONE   = 0,
    ACOVER_JPG    ,
    ACOVER_PNG    ,
}audio_cover_type;

typedef struct
{
    char title[512];
    char author[512];
    char album[512];
    char comment[512];
    char year[4];  
    int track;     
    char genre[32]; 
    char copyright[512];
    audio_cover_type pic; 
}audio_tag_info;

typedef struct
{    
    int id;
    int channel;
    int sample_rate;
    int bit_rate;
    aformat_t aformat;
    int duration;
	audio_tag_info *audio_tag;    
}maudio_info_t;

typedef struct
{
    char id;
    char internal_external; //0:internal_sub 1:external_sub       
    unsigned short width;
    unsigned short height;
	unsigned int sub_type;
    char resolution;
    long long subtitle_size;  
    char *sub_language;   
}msub_info_t;

typedef struct
{	
    char *filename;
    int  duration;  
	long long  file_size;
    pfile_type type;
	int bitrate;
    int has_video;
    int has_audio;
    int has_sub;
    int nb_streams;
    int total_video_num;
    int cur_video_index;
    int total_audio_num;
    int cur_audio_index;
    int total_sub_num;      
    int cur_sub_index;	
    int seekable;
    int drm_check;
}mstream_info_t;

typedef struct
{	
	mstream_info_t stream_info;
	mvideo_info_t *video_info[MAX_VIDEO_STREAMS];
	maudio_info_t *audio_info[MAX_AUDIO_STREAMS];
    msub_info_t *sub_info[MAX_SUB_STREAMS];
}media_info_t;

typedef struct player_info
{
	char *name;
	player_status last_sta;
	player_status status;		   /*stop,pause	*/
	int full_time;	   /*Seconds	*/
    int full_time_ms;  /* mSeconds */
	int current_time;  /*Seconds	*/
	int current_ms;	/*ms*/
	int last_time;		
	int error_no;  
	int start_time;
	int first_time;
	int pts_video;
	//int pts_pcrscr;
	unsigned int current_pts;
	long curtime_old_time;    
	unsigned int video_error_cnt;
	unsigned int audio_error_cnt;
	float audio_bufferlevel; // relative value
	float video_bufferlevel; // relative value
	int64_t	bufed_pos;
	int	bufed_time;/* Second*/
    unsigned int drm_rental;
}player_info_t;

typedef struct pid_info
{
    int num;
    int pid[MAX_PLAYER_THREADS];
}pid_info_t;

typedef struct player_file_type
{
	const char *fmt_string;
	int video_tracks;
	int audio_tracks;
	int subtitle_tracks;
	/**/
}player_file_type_t;


#define STATE_PRE(sta) (sta>>16)
#define PLAYER_THREAD_IS_INITING(sta)	(STATE_PRE(sta)==0x1)
#define PLAYER_THREAD_IS_RUNNING(sta)	(STATE_PRE(sta)==0x2)
#define PLAYER_THREAD_IS_STOPPED(sta)	(STATE_PRE(sta)==0x3)

typedef int (*update_state_fun_t)(int pid,player_info_t *) ;
typedef int (*notify_callback)(int pid,int msg,unsigned long ext1,unsigned long ext2);
typedef enum
{      
	PLAYER_EVENTS_PLAYER_INFO=1,			///<ext1=player_info*,ext2=0,same as update_statue_callback 
	PLAYER_EVENTS_STATE_CHANGED,			///<ext1=new_state,ext2=0,
	PLAYER_EVENTS_ERROR,					///<ext1=error_code,ext2=message char *
	PLAYER_EVENTS_BUFFERING,				///<ext1=buffered=d,d={0-100},ext2=0,
	PLAYER_EVENTS_FILE_TYPE,				///<ext1=player_file_type_t*,ext2=0
}player_events;

typedef struct
{
    update_state_fun_t update_statue_callback;
    int update_interval;
    long callback_old_time;
	notify_callback	   notify_fn;
}callback_t;

typedef struct
 {
	char  *file_name;						//file url
    char  *headers;							//file name's authentication information,maybe used in network streaming
	//List  *play_list;
	int	video_index;						//video track, no assigned, please set to -1
	int	audio_index;						//audio track, no assigned, please set to -1
	int sub_index;							//subtitle track, no assigned, please set to -1
	int t_pos;                  //start postion, use second as unit
	int	read_max_cnt;						//read retry maxium counts, if exceed it, return error
	int avsync_threshold;                             //for adec av sync threshold in ms
	union
	{     
		struct{
			unsigned int loop_mode:1;		//file loop mode 0:loop 1:not loop
			unsigned int nosound:1;			//0:play with audio  1:play without audio
			unsigned int novideo:1;			//0:play with video  1:play without video
			unsigned int hassub:1;			//0:ignore subtitle	 1:extract subtitle if have
			unsigned int need_start:1;/*If set need_start, we need call	player_start_play to playback*/
			#ifdef DEBUG_VARIABLE_DUR
			unsigned int is_variable:1;		//0:extrack duration from header 1:update duration during playback
			#endif
			unsigned int displast_frame : 1;//0:black out when player exit	1:keep last frame when player exit
		};
		int mode;							//no use
	};  
	callback_t callback_fn;					//callback function
	int byteiobufsize;						//byteio buffer size used in ffmpeg
	int loopbufsize;						//loop buffer size used in ffmpeg
	int enable_rw_on_pause;					//no use
	/*
	data%<min && data% <max  enter buffering;
	data% >middle exit buffering;
	*/
	int auto_buffing_enable;				 //auto buffering switch
	float buffing_min;						 //auto buffering low limit
	float buffing_middle;					 //auto buffering middle limit
	float buffing_max;						 //auto buffering high limit
	int is_playlist;						 //no use
	int is_type_parser;						 //is try to get file type 
	int buffing_starttime_s;			//for rest buffing_middle,buffering seconds data to start.
	int buffing_force_delay_s;
	int reserved [56];					//reserved  for furthur used,some one add more ,can del reserved num
 }play_control_t; 

#endif
