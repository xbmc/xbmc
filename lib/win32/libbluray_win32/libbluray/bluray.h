/*
 * This file is part of libbluray
 * Copyright (C) 2009-2010  Obliter0n
 * Copyright (C) 2009-2010  John Stebbins
 * Copyright (C) 2010       hpi1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef BLURAY_H_
#define BLURAY_H_

/**
 * @file libbluray/bluray.h
 * external API header
 */

#include <stdint.h>

#define TITLES_ALL              0    /**< all titles. */
#define TITLES_FILTER_DUP_TITLE 0x01 /**< remove duplicate titles. */
#define TITLES_FILTER_DUP_CLIP  0x02 /**< remove titles that have duplicate
                                          clips. */
#define TITLES_RELEVANT \
  (TITLES_FILTER_DUP_TITLE | TITLES_FILTER_DUP_CLIP) /**< remove duplicate
                                                          titles and clips */

typedef struct bluray BLURAY;

typedef enum {
    BLURAY_STREAM_TYPE_VIDEO_MPEG1        = 0x01,
    BLURAY_STREAM_TYPE_VIDEO_MPEG2        = 0x02,
    BLURAY_STREAM_TYPE_AUDIO_MPEG1        = 0x03,
    BLURAY_STREAM_TYPE_AUDIO_MPEG2        = 0x04,
    BLURAY_STREAM_TYPE_AUDIO_LPCM         = 0x80,
    BLURAY_STREAM_TYPE_AUDIO_AC3          = 0x81,
    BLURAY_STREAM_TYPE_AUDIO_DTS          = 0x82,
    BLURAY_STREAM_TYPE_AUDIO_TRUHD        = 0x83,
    BLURAY_STREAM_TYPE_AUDIO_AC3PLUS      = 0x84,
    BLURAY_STREAM_TYPE_AUDIO_DTSHD        = 0x85,
    BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER = 0x86,
    BLURAY_STREAM_TYPE_VIDEO_VC1          = 0xea,
    BLURAY_STREAM_TYPE_VIDEO_H264         = 0x1b,
    BLURAY_STREAM_TYPE_SUB_PG             = 0x90,
    BLURAY_STREAM_TYPE_SUB_IG             = 0x91,
    BLURAY_STREAM_TYPE_SUB_TEXT           = 0x92
} bd_stream_type_e;

typedef enum {
    BLURAY_VIDEO_FORMAT_480I              = 1,  // ITU-R BT.601-5
    BLURAY_VIDEO_FORMAT_576I              = 2,  // ITU-R BT.601-4
    BLURAY_VIDEO_FORMAT_480P              = 3,  // SMPTE 293M
    BLURAY_VIDEO_FORMAT_1080I             = 4,  // SMPTE 274M
    BLURAY_VIDEO_FORMAT_720P              = 5,  // SMPTE 296M
    BLURAY_VIDEO_FORMAT_1080P             = 6,  // SMPTE 274M
    BLURAY_VIDEO_FORMAT_576P              = 7   // ITU-R BT.1358
} bd_video_format_e;

typedef enum {
    BLURAY_VIDEO_RATE_24000_1001          = 1,  // 23.976
    BLURAY_VIDEO_RATE_24                  = 2,
    BLURAY_VIDEO_RATE_25                  = 3,
    BLURAY_VIDEO_RATE_30000_1001          = 4,  // 29.97
    BLURAY_VIDEO_RATE_50                  = 6,
    BLURAY_VIDEO_RATE_60000_1001          = 7   // 59.94
} bd_video_rate_e;

typedef enum {
    BLURAY_ASPECT_RATIO_4_3               = 2,
    BLURAY_ASPECT_RATIO_16_9              = 3
} bd_video_aspect_e;

typedef enum {
    BLURAY_AUDIO_FORMAT_MONO              = 1,
    BLURAY_AUDIO_FORMAT_STEREO            = 3,
    BLURAY_AUDIO_FORMAT_MULTI_CHAN        = 6,
    BLURAY_AUDIO_FORMAT_COMBO             = 12  // Stereo ac3/dts, 
} bd_audio_format_e;
                                                // multi mlp/dts-hd

typedef enum {
    BLURAY_AUDIO_RATE_48                  = 1,
    BLURAY_AUDIO_RATE_96                  = 4,
    BLURAY_AUDIO_RATE_192                 = 5,
    BLURAY_AUDIO_RATE_192_COMBO           = 12, // 48 or 96 ac3/dts
                                                // 192 mpl/dts-hd
    BLURAY_AUDIO_RATE_96_COMBO            = 14  // 48 ac3/dts
                                                // 96 mpl/dts-hd
} bd_audio_rate_e;

typedef enum {
    BLURAY_TEXT_CHAR_CODE_UTF8            = 0x01,
    BLURAY_TEXT_CHAR_CODE_UTF16BE         = 0x02,
    BLURAY_TEXT_CHAR_CODE_SHIFT_JIS       = 0x03,
    BLURAY_TEXT_CHAR_CODE_EUC_KR          = 0x04,
    BLURAY_TEXT_CHAR_CODE_GB18030_20001   = 0x05,
    BLURAY_TEXT_CHAR_CODE_CN_GB           = 0x06,
    BLURAY_TEXT_CHAR_CODE_BIG5            = 0x07
} bd_char_code_e;

typedef enum {
    BLURAY_STILL_NONE     = 0x00,
    BLURAY_STILL_TIME     = 0x01,
    BLURAY_STILL_INFINITE = 0x02,
} bd_still_mode_e;

typedef struct bd_stream_info {
    uint8_t     coding_type;
    uint8_t     format;
    uint8_t     rate;
    uint8_t     char_code;
    uint8_t     lang[4];
    uint16_t    pid;
    uint8_t     aspect;
} BLURAY_STREAM_INFO;

typedef struct bd_clip {
    uint32_t           pkt_count;
    uint8_t            still_mode;
    uint16_t           still_time;  /* seconds */
    uint8_t            video_stream_count;
    uint8_t            audio_stream_count;
    uint8_t            pg_stream_count;
    uint8_t            ig_stream_count;
    uint8_t            sec_audio_stream_count;
    uint8_t            sec_video_stream_count;
    BLURAY_STREAM_INFO *video_streams;
    BLURAY_STREAM_INFO *audio_streams;
    BLURAY_STREAM_INFO *pg_streams;
    BLURAY_STREAM_INFO *ig_streams;
    BLURAY_STREAM_INFO *sec_audio_streams;
    BLURAY_STREAM_INFO *sec_video_streams;
} BLURAY_CLIP_INFO;

typedef struct bd_chapter {
    uint32_t    idx;
    uint64_t    start;
    uint64_t    duration;
    uint64_t    offset;
} BLURAY_TITLE_CHAPTER;

typedef struct bd_title_info {
    uint32_t             idx;
    uint32_t             playlist;
    uint64_t             duration;
    uint32_t             clip_count;
    uint8_t              angle_count;
    uint32_t             chapter_count;
    BLURAY_CLIP_INFO     *clips;
    BLURAY_TITLE_CHAPTER *chapters;
} BLURAY_TITLE_INFO;

/**
 *
 *  This must be called after bd_open() and before bd_select_title().
 *  Populates the title list in BLURAY.
 *  Filtering of the returned list is controled through title flags
 *
 * @param bd  BLURAY object
 * @param flags  title flags
 * @return number of titles found
 */
uint32_t bd_get_titles(BLURAY *bd, uint8_t flags);

/**
 *
 *  Get information about a title
 *
 * @param bd  BLURAY object
 * @param title_idx title index number
 * @return allocated BLURAY_TITLE_INFO object, NULL on error
 */
BLURAY_TITLE_INFO* bd_get_title_info(BLURAY *bd, uint32_t title_idx);

/**
 *
 *  Get information about a playlist
 *
 * @param bd  BLURAY object
 * @param playlist playlist number
 * @return allocated BLURAY_TITLE_INFO object, NULL on error
 */
BLURAY_TITLE_INFO* bd_get_playlist_info(BLURAY *bd, uint32_t playlist);

/**
 *
 *  Free BLURAY_TITLE_INFO object
 *
 * @param title_info  BLURAY_TITLE_INFO object
 */
void bd_free_title_info(BLURAY_TITLE_INFO *title_info);

/**
 *  Initializes libbluray objects
 *
 * @param device_path   path to mounted Blu-ray disc
 * @param keyfile_path  path to KEYDB.cfg (may be NULL)
 * @return allocated BLURAY object, NULL if error
 */
BLURAY *bd_open(const char* device_path, const char* keyfile_path);

/**
 *  Free libbluray objects
 *
 * @param bd  BLURAY object
 */
void bd_close(BLURAY *bd);

/**
 *  Seek to pos in corrently selected title
 *
 * @param bd  BLURAY object
 * @param pos position to seek to
 * @return current seek position
 */
int64_t bd_seek(BLURAY *bd, uint64_t pos);

/**
 *
 * Seek to specific time in 90Khz ticks
 *
 * @param bd    BLURAY ojbect
 * @param tick  tick count
 * @return current seek position
 */
int64_t bd_seek_time(BLURAY *bd, uint64_t tick);

/**
 *
 *  Read from currently selected title file, decrypt if possible
 *
 * @param bd  BLURAY object
 * @param buf buffer to read data into
 * @param len size of data to be read
 * @return size of data read, -1 if error
 */
int bd_read(BLURAY *bd, unsigned char *buf, int len);

/**
 *
 *  Seek to a chapter. First chapter is 0
 *
 * @param bd  BLURAY object
 * @param chapter chapter to seek to
 * @return current seek position
 */
int64_t bd_seek_chapter(BLURAY *bd, unsigned chapter);

/**
 *
 *  Find the byte position of a chapter
 *
 * @param bd  BLURAY object
 * @param chapter chapter to find position of
 * @return seek position of chapter start
 */
int64_t bd_chapter_pos(BLURAY *bd, unsigned chapter);

/**
 *
 *  Get the current chapter
 *
 * @param bd  BLURAY object
 * @return current chapter
 */
uint32_t bd_get_current_chapter(BLURAY *bd);

/**
 *
 * Seek to a playmark. First mark is 0
 *
 * @param bd  BLURAY object
 * @param mark playmark to seek to
 * @return current seek position
 */
int64_t bd_seek_mark(BLURAY *bd, unsigned mark);

/**
 *
 *  Select a playlist
 *
 * @param bd  BLURAY object
 * @param playlist playlist to select
 * @return 1 on success, 0 if error
 */
int bd_select_playlist(BLURAY *bd, uint32_t playlist);

/**
 *
 *  Select the title from the list created by bd_get_titles()
 *
 * @param bd  BLURAY object
 * @param title title to select
 * @return 1 on success, 0 if error
 */
int bd_select_title(BLURAY *bd, uint32_t title);

/**
 *
 *  Set the angle to play
 *
 * @param bd  BLURAY object
 * @param angle angle to play
 * @return 1 on success, 0 if error
 */
int bd_select_angle(BLURAY *bd, unsigned angle);

/**
 *
 *  Initiate seamless angle change
 *
 * @param bd  BLURAY object
 * @param angle angle to change to
 */
void bd_seamless_angle_change(BLURAY *bd, unsigned angle);

/**
 *
 *  Returns file size in bytes of currently selected title, 0 in no title
 *  selected
 *
 * @param bd  BLURAY object
 * @return file size in bytes of currently selected title, 0 if no title
 * selected
 */
uint64_t bd_get_title_size(BLURAY *bd);

/**
 *
 *  Returns the current title index
 *
 * @param bd  BLURAY object
 * @return current title index
 */
uint32_t bd_get_current_title(BLURAY *bd);

/**
 *
 *  Return the current angle
 *
 * @param bd  BLURAY object
 * @return current angle
 */
unsigned bd_get_current_angle(BLURAY *bd);

/**
 *
 *  Return current pos
 *
 * @param bd  BLURAY object
 * @return current seek position
 */
uint64_t bd_tell(BLURAY *bd);

/**
 *
 *  Return current time
 *
 * @param bd  BLURAY object
 * @return current time
 */
uint64_t bd_tell_time(BLURAY *bd);

/*
 * Disc info
 */

typedef struct {
    uint8_t  bluray_detected;

    uint8_t  first_play_supported;
    uint8_t  top_menu_supported;

    uint32_t num_hdmv_titles;
    uint32_t num_bdj_titles;
    uint32_t num_unsupported_titles;

    uint8_t  aacs_detected;
    uint8_t  libaacs_detected;
    uint8_t  aacs_handled;

    uint8_t  bdplus_detected;
    uint8_t  libbdplus_detected;
    uint8_t  bdplus_handled;

} BLURAY_DISC_INFO;

/**
 *
 *  Get information about current BluRay disc
 *
 * @param bd  BLURAY object
 * @return pointer to BLURAY_DISC_INFO object, NULL on error
 */
const BLURAY_DISC_INFO *bd_get_disc_info(BLURAY*);

/*
 * player settings
 */

typedef enum {
    BLURAY_PLAYER_SETTING_PARENTAL       = 13,  /* Age for parental control (years) */
    BLURAY_PLAYER_SETTING_AUDIO_CAP      = 15,  /* Player capability for audio (bit mask) */
    BLURAY_PLAYER_SETTING_AUDIO_LANG     = 16,  /* Initial audio language: ISO 639-2 string, ex. "eng" */
    BLURAY_PLAYER_SETTING_PG_LANG        = 17,  /* Initial PG/SPU language: ISO 639-2 string, ex. "eng" */
    BLURAY_PLAYER_SETTING_MENU_LANG      = 18,  /* Initial menu language: ISO 639-2 string, ex. "eng" */
    BLURAY_PLAYER_SETTING_COUNTRY_CODE   = 19,  /* Player country code: ISO 3166-1 string, ex. "de" */
    BLURAY_PLAYER_SETTING_REGION_CODE    = 20,  /* Player region code: 1 - region A, 2 - B, 4 - C */
    BLURAY_PLAYER_SETTING_VIDEO_CAP      = 29,  /* Player capability for video (bit mask) */
    BLURAY_PLAYER_SETTING_TEXT_CAP       = 30,  /* Player capability for text subtitle (bit mask) */
    BLURAY_PLAYER_SETTING_PLAYER_PROFILE = 31,  /* Profile1: 0, Profile1+: 1, Profile2: 3, Profile3: 8 */
} bd_player_setting;

/**
 *
 *  Update player setting registers
 *
 * @param bd  BLURAY object
 * @param idx Player setting register
 * @param value New value for player setting register
 * @return 1 on success, 0 on error (invalid setting)
 */

int bd_set_player_setting(BLURAY *bd, uint32_t idx, uint32_t value);
int bd_set_player_setting_str(BLURAY *bd, uint32_t idx, const char *s);

/*
 * Java
 */
int bd_start_bdj(BLURAY *bd, const char* start_object); // start BD-J from the specified BD-J object (should be a 5 character string)
void bd_stop_bdj(BLURAY *bd); // shutdown BD-J and clean up resources

/*
 * events
 */

typedef enum {
    BD_EVENT_NONE = 0,
    BD_EVENT_ERROR,
    BD_EVENT_ENCRYPTED,

    /* current playback position */
    BD_EVENT_ANGLE,     /* current angle, 1...N */
    BD_EVENT_TITLE,     /* current title, 1...N (0 = top menu) */
    BD_EVENT_PLAYLIST,  /* current playlist (xxxxx.mpls) */
    BD_EVENT_PLAYITEM,  /* current play item */
    BD_EVENT_CHAPTER,   /* current chapter, 1...N */
    BD_EVENT_END_OF_TITLE,

    /* stream selection */
    BD_EVENT_AUDIO_STREAM,           /* 1..32,  0xff  = none */
    BD_EVENT_IG_STREAM,              /* 1..32                */
    BD_EVENT_PG_TEXTST_STREAM,       /* 1..255, 0xfff = none */
    BD_EVENT_PIP_PG_TEXTST_STREAM,   /* 1..255, 0xfff = none */
    BD_EVENT_SECONDARY_AUDIO_STREAM, /* 1..32,  0xff  = none */
    BD_EVENT_SECONDARY_VIDEO_STREAM, /* 1..32,  0xff  = none */

    BD_EVENT_PG_TEXTST,              /* 0 - disable, 1 - enable */
    BD_EVENT_PIP_PG_TEXTST,          /* 0 - disable, 1 - enable */
    BD_EVENT_SECONDARY_AUDIO,        /* 0 - disable, 1 - enable */
    BD_EVENT_SECONDARY_VIDEO,        /* 0 - disable, 1 - enable */
    BD_EVENT_SECONDARY_VIDEO_SIZE,   /* 0 - PIP, 0xf - fullscreen */

    /* HDMV VM or JVM seeked the stream. Next read() will return data from new position. */
    BD_EVENT_SEEK,

    /* still playback (pause) */
    BD_EVENT_STILL,                  /* 0 - off, 1 - on */

    /* Still playback for n seconds (reached end of still mode play item) */
    BD_EVENT_STILL_TIME,             /* 0 = infinite ; 1...300 = seconds */

} bd_event_e;

typedef struct {
    uint32_t   event;  /* bd_event_e */
    uint32_t   param;
} BD_EVENT;

/**
 *
 *  Get event from libbluray event queue.
 *
 * @param bd  BLURAY object
 * @param event next BD_EVENT from event queue
 * @return 1 on success, 0 if no events
 */
int  bd_get_event(BLURAY *bd, BD_EVENT *event);

/*
 * navigaton mode
 */

/**
 *
 *  Start playing disc in navigation mode.
 *
 *  Playback is started from "First Play" title.
 *
 * @param bd  BLURAY object
 * @return 1 on success, 0 if error
 */
int  bd_play(BLURAY *bd);

/**
 *
 *  Read from currently playing title.
 *
 *  When playing disc in navigation mode this function must be used instead of bd_read().
 *
 * @param bd  BLURAY object
 * @param buf buffer to read data into
 * @param len size of data to be read
 * @param event next BD_EVENT from event queue (BD_EVENT_NONE if no events)
 * @return size of data read, -1 if error, 0 if event needs to be handled first, 0 if end of title was reached
 */
int  bd_read_ext(BLURAY *bd, unsigned char *buf, int len, BD_EVENT *event);

/**
 *
 *  Play a title (from disc index).
 *
 *  Title 0      = Top Menu
 *  Title 0xffff = First Play title
 *  Number of titles can be found from BLURAY_DISC_INFO.
 *
 * @param bd  BLURAY object
 * @param title title number from disc index
 * @return 1 on success, 0 if error
 */
int  bd_play_title(BLURAY *bd, unsigned title);

/**
 *
 *  Open BluRay disc Top Menu.
 *
 *  Current pts is needed for resuming playback when menu is closed.
 *
 * @param bd  BLURAY object
 * @param pts current playback position (1/90000s) or -1
 * @return 1 on success, 0 if error
 */
int  bd_menu_call(BLURAY *bd, int64_t pts);

/*
 * User interaction and On-screen display controller
 */

struct bd_overlay_s; /* defined in overlay.h */
typedef void (*bd_overlay_proc_f)(void *, const struct bd_overlay_s * const);

/**
 *
 *  Register overlay graphics handler function.
 *
 * @param bd  BLURAY object
 * @param handle application-specific handle that will be passed to handler function
 * @param func handler function pointer
 * @return 1 on success, 0 if error
 */
void bd_register_overlay_proc(BLURAY *bd, void *handle, bd_overlay_proc_f func);

/**
 *
 *  Pass user input to graphics controller.
 *  Keys are defined in libbluray/keys.h.
 *  Current pts can be updated by using BD_VK_NONE key. This is required for animated menus.
 *
 * @param bd  BLURAY object
 * @param pts current playback position (1/90000s) or -1
 * @param key input key
 * @return 1 on success, 0 if error
 */
void bd_user_input(BLURAY *bd, int64_t pts, uint32_t key);

/**
 *
 *  Select menu button at location (x,y).
 *
 * @param bd  BLURAY object
 * @param pts current playback position (1/90000s) or -1
 * @param x mouse pointer x-position
 * @param y mouse pointer y-position
 * @return none
 */
void bd_mouse_select(BLURAY *bd, int64_t pts, uint16_t x, uint16_t y);

struct meta_dl;
/**
 *
 *  Get meta information about the bluray disc.
 *
 * @param bd  BLURAY object
 * @return allocated META_DL (disclib) object, NULL on error
 */
struct meta_dl *bd_get_meta(BLURAY *bd);

#endif /* BLURAY_H_ */
