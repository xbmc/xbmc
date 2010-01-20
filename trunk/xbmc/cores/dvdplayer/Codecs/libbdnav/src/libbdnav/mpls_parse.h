#if !defined(_MPLS_PARSE_H_)
#define _MPLS_PARSE_H_

#include <stdio.h>
#include <stdint.h>

#define BD_MARK_ENTRY   0x01
#define BD_MARK_LINK    0x02

typedef struct
{
    uint8_t         menu_call : 1;
    uint8_t         title_search : 1;
    uint8_t         chapter_search : 1;
    uint8_t         time_search : 1;
    uint8_t         skip_to_next_point : 1;
    uint8_t         skip_to_prev_point : 1;
    uint8_t         play_firstplay : 1;
    uint8_t         stop : 1;
    uint8_t         pause_on : 1;
    uint8_t         pause_off : 1;
    uint8_t         still : 1;
    uint8_t         forward : 1;
    uint8_t         backward : 1;
    uint8_t         resume : 1;
    uint8_t         move_up : 1;
    uint8_t         move_down : 1;
    uint8_t         move_left : 1;
    uint8_t         move_right : 1;
    uint8_t         select : 1;
    uint8_t         activate : 1;
    uint8_t         select_and_activate : 1;
    uint8_t         primary_audio_change : 1;
    uint8_t         angle_change : 1;
    uint8_t         popup_on : 1;
    uint8_t         popup_off : 1;
    uint8_t         pg_enable_disable : 1;
    uint8_t         pg_change : 1;
    uint8_t         secondary_video_enable_disable : 1;
    uint8_t         secondary_video_change : 1;
    uint8_t         secondary_audio_enable_disable : 1;
    uint8_t         secondary_audio_change : 1;
    uint8_t         pip_pg_change : 1;
} MPLS_UO;

typedef struct
{
    uint8_t         stream_type;
    uint8_t         coding_type;
    uint16_t        pid;
    uint8_t         subpath_id;
    uint8_t         subclip_id;
    uint8_t         format;
    uint8_t         rate;
    uint8_t         char_code;
    uint8_t         lang[4];
} MPLS_STREAM;

typedef struct
{
    uint8_t         num_video;
    uint8_t         num_audio;
    uint8_t         num_pg;
    uint8_t         num_ig;
    uint8_t         num_secondary_audio;
    uint8_t         num_secondary_video;
    uint8_t         num_pip_pg;
    MPLS_STREAM    *video;
    MPLS_STREAM    *audio;
    MPLS_STREAM    *pg;
} MPLS_STN;

typedef struct
{
    char            clip_id[6];
    char            codec_id[5];
    uint8_t         stc_id;
} MPLS_CLIP;

typedef struct
{
    uint8_t         is_multi_angle;
    uint8_t         connection_condition;
    uint32_t        in_time;
    uint32_t        out_time;
    MPLS_UO         uo_mask;
    uint8_t         random_access_flag;
    uint8_t         still_mode;
    uint16_t        still_time;
    uint8_t         angle_count;
    uint8_t         is_different_audio;
    uint8_t         is_seamless_angle;
    MPLS_CLIP       *clip;
    MPLS_STN        stn;
} MPLS_PI;

typedef struct
{
    uint8_t         mark_id;
    uint8_t         mark_type;
    uint16_t        play_item_ref;
    uint32_t        time;
    uint16_t        entry_es_pid;
    uint32_t        duration;
} MPLS_PLM;

typedef struct
{
    uint8_t         playback_type;
    uint16_t        playback_count;
    MPLS_UO         uo_mask;
    uint8_t         random_access_flag;
    uint8_t         audio_mix_flag;
    uint8_t         lossless_bypass_flag;
} MPLS_AI;

typedef struct
{
    uint8_t         connection_condition;
    uint8_t         is_multi_clip;
    uint32_t        in_time;
    uint32_t        out_time;
    uint16_t        sync_play_item_id;
    uint32_t        sync_pts;
    uint8_t         clip_count;
    MPLS_CLIP       *clip;
} MPLS_SUB_PI;

typedef struct
{
    uint8_t         type;
    uint8_t         is_repeat;
    uint8_t         sub_playitem_count;
    MPLS_SUB_PI     *sub_play_item;
} MPLS_SUB;

typedef struct
{
    uint32_t        type_indicator;
    uint32_t        type_indicator2;
    uint32_t        list_pos;
    uint32_t        mark_pos;
    uint32_t        ext_pos;
    MPLS_AI         app_info;
    uint16_t        list_count;
    uint16_t        sub_count;
    uint16_t        mark_count;
    MPLS_PI        *play_item;
    MPLS_SUB       *sub_path;
    MPLS_PLM       *play_mark;
} MPLS_PL;


MPLS_PL* mpls_parse(char *path, int verbose);
void mpls_free(MPLS_PL *pl);

#endif // _MPLS_PARSE_H_
