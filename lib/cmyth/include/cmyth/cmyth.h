/*
 *  Copyright (C) 2004-2012, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*! \mainpage cmyth
 *
 * cmyth is a library that provides a C language API to access and control
 * a MythTV backend.
 *
 * \section projectweb Project website
 * http://www.mvpmc.org/
 *
 * \section repos Source repositories
 * http://git.mvpmc.org/
 *
 * \section libraries Libraries
 * \li \link cmyth.h libcmyth \endlink
 * \li \link refmem.h librefmem \endlink
 */

/** \file cmyth.h
 * A C library for communicating with a MythTV server.
 */

#ifndef __CMYTH_H
#define __CMYTH_H

#ifdef __APPLE__ 
#include <sys/time.h>
#else
#include <time.h>
#endif

/*
 * -----------------------------------------------------------------
 * Types
 * -----------------------------------------------------------------
 */
struct cmyth_conn;
typedef struct cmyth_conn *cmyth_conn_t;

/* Sergio: Added to support the new livetv protocol */
struct cmyth_livetv_chain;
typedef struct cmyth_livetv_chain *cmyth_livetv_chain_t;

struct cmyth_recorder;
typedef struct cmyth_recorder *cmyth_recorder_t;

struct cmyth_ringbuf;
typedef struct cmyth_ringbuf *cmyth_ringbuf_t;

struct cmyth_rec_num;
typedef struct cmyth_rec_num *cmyth_rec_num_t;

struct cmyth_posmap;
typedef struct cmyth_posmap *cmyth_posmap_t;

struct cmyth_proginfo;
typedef struct cmyth_proginfo *cmyth_proginfo_t;

struct cmyth_database;
typedef struct cmyth_database *cmyth_database_t;


typedef enum {
	CHANNEL_DIRECTION_UP = 0,
	CHANNEL_DIRECTION_DOWN = 1,
	CHANNEL_DIRECTION_FAVORITE = 2,
	CHANNEL_DIRECTION_SAME = 4,
} cmyth_channeldir_t;

typedef enum {
	ADJ_DIRECTION_UP = 1,
	ADJ_DIRECTION_DOWN = 0,
} cmyth_adjdir_t;

typedef enum {
	BROWSE_DIRECTION_SAME = 0,
	BROWSE_DIRECTION_UP = 1,
	BROWSE_DIRECTION_DOWN = 2,
	BROWSE_DIRECTION_LEFT = 3,
	BROWSE_DIRECTION_RIGHT = 4,
	BROWSE_DIRECTION_FAVORITE = 5,
} cmyth_browsedir_t;

typedef enum {
	WHENCE_SET = 0,
	WHENCE_CUR = 1,
	WHENCE_END = 2,
} cmyth_whence_t;

typedef enum {
	CMYTH_EVENT_UNKNOWN = 0,
	CMYTH_EVENT_CLOSE = 1,
	CMYTH_EVENT_RECORDING_LIST_CHANGE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE,
	CMYTH_EVENT_SCHEDULE_CHANGE,
	CMYTH_EVENT_DONE_RECORDING,
	CMYTH_EVENT_QUIT_LIVETV,
	CMYTH_EVENT_WATCH_LIVETV,
	CMYTH_EVENT_LIVETV_CHAIN_UPDATE,
	CMYTH_EVENT_SIGNAL,
	CMYTH_EVENT_ASK_RECORDING,
	CMYTH_EVENT_SYSTEM_EVENT,
	CMYTH_EVENT_UPDATE_FILE_SIZE,
	CMYTH_EVENT_GENERATED_PIXMAP,
	CMYTH_EVENT_CLEAR_SETTINGS_CACHE,
} cmyth_event_t;

#define CMYTH_NUM_SORTS 2
typedef enum {
	MYTHTV_SORT_DATE_RECORDED = 0,
	MYTHTV_SORT_ORIGINAL_AIRDATE,
} cmyth_proglist_sort_t;

struct cmyth_timestamp;
typedef struct cmyth_timestamp *cmyth_timestamp_t;

struct cmyth_keyframe;
typedef struct cmyth_keyframe *cmyth_keyframe_t;

struct cmyth_freespace;
typedef struct cmyth_freespace *cmyth_freespace_t;

struct cmyth_proglist;
typedef struct cmyth_proglist *cmyth_proglist_t;

struct cmyth_file;
typedef struct cmyth_file *cmyth_file_t;

struct cmyth_commbreak {
        long long start_mark;
        long long start_offset;
        long long end_mark;
        long long end_offset;
};
typedef struct cmyth_commbreak *cmyth_commbreak_t;

struct cmyth_commbreaklist {
        cmyth_commbreak_t *commbreak_list;
        long commbreak_count;
};
typedef struct cmyth_commbreaklist *cmyth_commbreaklist_t;

/* Sergio: Added to support the tvguide functionality */

struct cmyth_channel;
typedef struct cmyth_channel *cmyth_channel_t;

struct cmyth_chanlist;
typedef struct cmyth_chanlist *cmyth_chanlist_t;

struct cmyth_tvguide_progs;
typedef struct cmyth_tvguide_progs *cmyth_tvguide_progs_t;

/*
 * -----------------------------------------------------------------
 * Debug Output Control
 * -----------------------------------------------------------------
 */

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define CMYTH_DBG_NONE  -1
#define CMYTH_DBG_ERROR  0
#define CMYTH_DBG_WARN   1
#define CMYTH_DBG_INFO   2
#define CMYTH_DBG_DETAIL 3
#define CMYTH_DBG_DEBUG  4
#define CMYTH_DBG_PROTO  5
#define CMYTH_DBG_ALL    6

/**
 * Set the libcmyth debug level.
 * \param l level
 */
extern void cmyth_dbg_level(int l);

/**
 * Turn on all libcmyth debugging.
 */
extern void cmyth_dbg_all(void);

/**
 * Turn off all libcmyth debugging.
 */
extern void cmyth_dbg_none(void);

/**
 * Print a libcmyth debug message.
 * \param level debug level
 * \param fmt printf style format
 */
extern void cmyth_dbg(int level, char *fmt, ...);

/**
 * Define a callback to use to send messages rather than using stdout
 * \param msgcb function pointer to pass a string to
 */
extern void cmyth_set_dbg_msgcallback(void (*msgcb)(int level,char *));

/*
 * -----------------------------------------------------------------
 * Connection Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a control connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_connect_ctrl(char *server,
					    unsigned short port,
					    unsigned buflen, int tcp_rcvbuf);

/**
 * Create an event connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return event handle
 */
extern cmyth_conn_t cmyth_conn_connect_event(char *server,
					     unsigned short port,
					     unsigned buflen, int tcp_rcvbuf);

/**
 * Create a file connection to a backend for reading a recording.
 * \param prog program handle
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_file(cmyth_proginfo_t prog,
					    cmyth_conn_t control,
					    unsigned buflen, int tcp_rcvbuf);


/**
 * Create a file connection to a backend.
 * \param path path to file
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_path(char* path, cmyth_conn_t control,
					    unsigned buflen, int tcp_rcvbuf);

/**
 * Create a ring buffer connection to a recorder.
 * \param rec recorder handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \retval 0 success
 * \retval -1 error
 */
extern int cmyth_conn_connect_ring(cmyth_recorder_t rec, unsigned buflen,
				   int tcp_rcvbuf);

/**
 * Create a connection to a recorder.
 * \param rec recorder to connect to
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \retval 0 success
 * \retval -1 error
 */
extern int cmyth_conn_connect_recorder(cmyth_recorder_t rec,
				       unsigned buflen, int tcp_rcvbuf);

/**
 * Check whether a block has finished transfering from a backend.
 * \param conn control handle
 * \param size size of block
 * \retval 0 not complete
 * \retval 1 complete
 * \retval <0 error
 */
extern int cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size);

/**
 * Obtain a recorder from a connection by its recorder number.
 * \param conn connection handle
 * \param num recorder number
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_recorder_from_num(cmyth_conn_t conn,
							 int num);

/**
 * Obtain the next available free recorder on a backend.
 * \param conn connection handle
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_free_recorder(cmyth_conn_t conn);

/**
 * Get the amount of free disk space on a backend.
 * \param control control handle
 * \param[out] total total disk space
 * \param[out] used used disk space
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_conn_get_freespace(cmyth_conn_t control,
				    long long *total, long long *used);

/**
 * Determine if a control connection is not responding.
 * \param control control handle
 * \retval 0 not hung
 * \retval 1 hung
 * \retval <0 error
 */
extern int cmyth_conn_hung(cmyth_conn_t control);

/**
 * Determine the number of free recorders.
 * \param conn connection handle
 * \return number of free recorders
 */
extern int cmyth_conn_get_free_recorder_count(cmyth_conn_t conn);

/**
 * Determine the MythTV protocol version being used.
 * \param conn connection handle
 * \return protocol version
 */
extern int cmyth_conn_get_protocol_version(cmyth_conn_t conn);

/**
 * Return a MythTV setting for a hostname
 * \param conn connection handle
 * \param hostname hostname to retreive the setting from
 * \param setting the setting name to get
 * \return ref counted string with the setting
 */
extern char * cmyth_conn_get_setting(cmyth_conn_t conn,
               const char* hostname, const char* setting);

/*
 * -----------------------------------------------------------------
 * Event Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve an event from a backend.
 * \param conn connection handle
 * \param[out] data data, if the event returns any
 * \param len size of data buffer
 * \return event type
 */
extern cmyth_event_t cmyth_event_get(cmyth_conn_t conn, char * data, int len);

/**
 * Selects on the event socket, waiting for an event to show up.
 * allows nonblocking access to events.
 * \return <= 0 on failure
 */
extern int cmyth_event_select(cmyth_conn_t conn, struct timeval *timeout);

/*
 * -----------------------------------------------------------------
 * Recorder Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a new recorder.
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_create(void);

/**
 * Duplicaate a recorder.
 * \param p recorder handle
 * \return duplicated recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_dup(cmyth_recorder_t p);

/**
 * Determine if a recorder is in use.
 * \param rec recorder handle
 * \retval 0 not recording
 * \retval 1 recording
 * \retval <0 error
 */
extern int cmyth_recorder_is_recording(cmyth_recorder_t rec);

/**
 * Determine the framerate for a recorder.
 * \param rec recorder handle
 * \param[out] rate framerate
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_get_framerate(cmyth_recorder_t rec,
					double *rate);

extern long long cmyth_recorder_get_frames_written(cmyth_recorder_t rec);

extern long long cmyth_recorder_get_free_space(cmyth_recorder_t rec);

extern long long cmyth_recorder_get_keyframe_pos(cmyth_recorder_t rec,
						 unsigned long keynum);

extern cmyth_posmap_t cmyth_recorder_get_position_map(cmyth_recorder_t rec,
						      unsigned long start,
						      unsigned long end);

extern cmyth_proginfo_t cmyth_recorder_get_recording(cmyth_recorder_t rec);

extern int cmyth_recorder_stop_playing(cmyth_recorder_t rec);

extern int cmyth_recorder_frontend_ready(cmyth_recorder_t rec);

extern int cmyth_recorder_cancel_next_recording(cmyth_recorder_t rec);

/**
 * Request that the recorder stop transmitting data.
 * \param rec recorder handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_pause(cmyth_recorder_t rec);

extern int cmyth_recorder_finish_recording(cmyth_recorder_t rec);

extern int cmyth_recorder_toggle_channel_favorite(cmyth_recorder_t rec);

/**
 * Request that the recorder change the channel being recorded.
 * \param rec recorder handle
 * \param direction direction in which to change channel
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_change_channel(cmyth_recorder_t rec,
					 cmyth_channeldir_t direction);

/**
 * Set the channel for a recorder.
 * \param rec recorder handle
 * \param channame channel name to change to
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_set_channel(cmyth_recorder_t rec,
				      char *channame);

extern int cmyth_recorder_change_color(cmyth_recorder_t rec,
				       cmyth_adjdir_t direction);

extern int cmyth_recorder_change_brightness(cmyth_recorder_t rec,
					    cmyth_adjdir_t direction);

extern int cmyth_recorder_change_contrast(cmyth_recorder_t rec,
					  cmyth_adjdir_t direction);

extern int cmyth_recorder_change_hue(cmyth_recorder_t rec,
				     cmyth_adjdir_t direction);

extern int cmyth_recorder_check_channel(cmyth_recorder_t rec,
					char *channame);

extern int cmyth_recorder_check_channel_prefix(cmyth_recorder_t rec,
					       char *channame);

/**
 * Request the current program info for a recorder.
 * \param rec recorder handle
 * \return program info handle
 */
extern cmyth_proginfo_t cmyth_recorder_get_cur_proginfo(cmyth_recorder_t rec);

/**
 * Request the next program info for a recorder.
 * \param rec recorder handle
 * \param current current program
 * \param direction direction of next program
 * \retval 0 success
 * \retval <0 error
 */
extern cmyth_proginfo_t cmyth_recorder_get_next_proginfo(
	cmyth_recorder_t rec,
	cmyth_proginfo_t current,
	cmyth_browsedir_t direction);

extern int cmyth_recorder_get_input_name(cmyth_recorder_t rec,
					 char *name,
					 unsigned len);

extern long long cmyth_recorder_seek(cmyth_recorder_t rec,
				     long long pos,
				     cmyth_whence_t whence,
				     long long curpos);

extern int cmyth_recorder_spawn_chain_livetv(cmyth_recorder_t rec, char* channame);

extern int cmyth_recorder_spawn_livetv(cmyth_recorder_t rec);

extern int cmyth_recorder_start_stream(cmyth_recorder_t rec);

extern int cmyth_recorder_end_stream(cmyth_recorder_t rec);
extern char*cmyth_recorder_get_filename(cmyth_recorder_t rec);
extern int cmyth_recorder_stop_livetv(cmyth_recorder_t rec);
extern int cmyth_recorder_done_ringbuf(cmyth_recorder_t rec);
extern int cmyth_recorder_get_recorder_id(cmyth_recorder_t rec);

/*
 * -----------------------------------------------------------------
 * Live TV Operations
 * -----------------------------------------------------------------
 */

extern cmyth_livetv_chain_t cmyth_livetv_chain_create(char * chainid);

extern cmyth_file_t cmyth_livetv_get_cur_file(cmyth_recorder_t rec);

extern int cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir);

extern int cmyth_livetv_chain_switch_last(cmyth_recorder_t rec);

extern int cmyth_livetv_chain_update(cmyth_recorder_t rec, char * chainid,
						int tcp_rcvbuf);

extern cmyth_recorder_t cmyth_spawn_live_tv(cmyth_recorder_t rec,
										unsigned buflen,
										int tcp_rcvbuf,
                    void (*prog_update_callback)(cmyth_proginfo_t),
										char ** err, char * channame);

extern cmyth_recorder_t cmyth_livetv_chain_setup(cmyth_recorder_t old_rec,
						 int tcp_rcvbuf,
						 void (*prog_update_callback)(cmyth_proginfo_t));

extern int cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf,
                                  unsigned long len);

extern int cmyth_livetv_select(cmyth_recorder_t rec, struct timeval *timeout);
  
extern int cmyth_livetv_request_block(cmyth_recorder_t rec, unsigned long len);

extern long long cmyth_livetv_seek(cmyth_recorder_t rec,
						long long offset, int whence);

extern int cmyth_livetv_read(cmyth_recorder_t rec,
			     char *buf,
			     unsigned long len);

extern int cmyth_livetv_keep_recording(cmyth_recorder_t rec, cmyth_database_t db, int keep);

extern int mythtv_new_livetv(void);
extern int cmyth_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_enabled);

/*
 * -----------------------------------------------------------------
 * Database Operations 
 * -----------------------------------------------------------------
 */

extern cmyth_database_t cmyth_database_init(char *host, char *db_name, char *user, char *pass);
extern cmyth_chanlist_t myth_tvguide_load_channels(cmyth_database_t db,
																									 int sort_desc);
extern int cmyth_database_set_host(cmyth_database_t db, char *host);
extern int cmyth_database_set_user(cmyth_database_t db, char *user);
extern int cmyth_database_set_pass(cmyth_database_t db, char *pass);
extern int cmyth_database_set_name(cmyth_database_t db, char *name);

/*
 * -----------------------------------------------------------------
 * Ring Buffer Operations
 * -----------------------------------------------------------------
 */
extern char * cmyth_ringbuf_pathname(cmyth_recorder_t rec);

extern cmyth_ringbuf_t cmyth_ringbuf_create(void);

extern cmyth_recorder_t cmyth_ringbuf_setup(cmyth_recorder_t old_rec);

extern int cmyth_ringbuf_request_block(cmyth_recorder_t rec,
				       unsigned long len);

extern int cmyth_ringbuf_select(cmyth_recorder_t rec, struct timeval *timeout);

extern int cmyth_ringbuf_get_block(cmyth_recorder_t rec,
				   char *buf,
				   unsigned long len);

extern long long cmyth_ringbuf_seek(cmyth_recorder_t rec,
				    long long offset,
				    int whence);

extern int cmyth_ringbuf_read(cmyth_recorder_t rec,
			      char *buf,
			      unsigned long len);
/*
 * -----------------------------------------------------------------
 * Recorder Number Operations
 * -----------------------------------------------------------------
 */
extern cmyth_rec_num_t cmyth_rec_num_create(void);

extern cmyth_rec_num_t cmyth_rec_num_get(char *host,
					 unsigned short port,
					 unsigned id);

extern char *cmyth_rec_num_string(cmyth_rec_num_t rn);

/*
 * -----------------------------------------------------------------
 * Timestamp Operations
 * -----------------------------------------------------------------
 */
extern cmyth_timestamp_t cmyth_timestamp_create(void);

extern cmyth_timestamp_t cmyth_timestamp_from_string(char *str);

extern cmyth_timestamp_t cmyth_timestamp_from_unixtime(time_t l);

extern time_t cmyth_timestamp_to_unixtime(cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_string(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_isostring(char *str, cmyth_timestamp_t ts);

extern int cmyth_timestamp_to_display_string(char *str, cmyth_timestamp_t ts,
																						 int time_format_12);

extern int cmyth_datetime_to_string(char *str, cmyth_timestamp_t ts);

extern cmyth_timestamp_t cmyth_datetime_from_string(char *str);

extern int cmyth_timestamp_compare(cmyth_timestamp_t ts1,
				   cmyth_timestamp_t ts2);

/*
 * -----------------------------------------------------------------
 * Key Frame Operations
 * -----------------------------------------------------------------
 */
extern cmyth_keyframe_t cmyth_keyframe_create(void);

extern cmyth_keyframe_t cmyth_keyframe_tcmyth_keyframe_get(
	unsigned long keynum,
	unsigned long long pos);

extern char *cmyth_keyframe_string(cmyth_keyframe_t kf);

/*
 * -----------------------------------------------------------------
 * Position Map Operations
 * -----------------------------------------------------------------
 */
extern cmyth_posmap_t cmyth_posmap_create(void);

/*
 * -----------------------------------------------------------------
 * Program Info Operations
 * -----------------------------------------------------------------
 */

/**
 * Program recording status.
 */
typedef enum {
	RS_DELETED = -5,
	RS_STOPPED = -4,
	RS_RECORDED = -3,
	RS_RECORDING = -2,
	RS_WILL_RECORD = -1,
	RS_DONT_RECORD = 1,
	RS_PREVIOUS_RECORDING = 2,
	RS_CURRENT_RECORDING = 3,
	RS_EARLIER_RECORDING = 4,
	RS_TOO_MANY_RECORDINGS = 5,
	RS_CANCELLED = 6,
	RS_CONFLICT = 7,
	RS_LATER_SHOWING = 8,
	RS_REPEAT = 9,
	RS_LOW_DISKSPACE = 11,
	RS_TUNER_BUSY = 12,
} cmyth_proginfo_rec_status_t;

/**
 * Create a new program info data structure.
 * \return proginfo handle
 */
extern cmyth_proginfo_t cmyth_proginfo_create(void);

extern int cmyth_proginfo_stop_recording(cmyth_conn_t control,
					 cmyth_proginfo_t prog);

extern int cmyth_proginfo_check_recording(cmyth_conn_t control,
					  cmyth_proginfo_t prog);

/**
 * Delete a program.
 * \param control backend control handle
 * \param prog proginfo handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_proginfo_delete_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog);

/**
 * Delete a program such that it may be recorded again.
 * \param control backend control handle
 * \param prog proginfo handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_proginfo_forget_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog);

extern int cmyth_proginfo_get_recorder_num(cmyth_conn_t control,
					   cmyth_rec_num_t rnum,
					   cmyth_proginfo_t prog);

extern cmyth_proginfo_t cmyth_proginfo_get_from_basename(cmyth_conn_t control,
							 const char* basename);

/**
 * Retrieve the title of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_title(cmyth_proginfo_t prog);

/**
 * Retrieve the subtitle of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_subtitle(cmyth_proginfo_t prog);

/**
 * Retrieve the description of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_description(cmyth_proginfo_t prog);

/**
 * Retrieve the season of a program.
 * \param prog proginfo handle
 * \return season
 */
extern unsigned short cmyth_proginfo_season(cmyth_proginfo_t prog);

/**
 * Retrieve the episode of a program.
 * \param prog proginfo handle
 * \return episode
 */
extern unsigned short cmyth_proginfo_episode(cmyth_proginfo_t prog);

/**
 * Retrieve the category of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_category(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanstr(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chansign(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_channame(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return channel number
 */
extern long cmyth_proginfo_chan_id(cmyth_proginfo_t prog);

/**
 * Retrieve the pathname of a program file.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_pathname(cmyth_proginfo_t prog);

/**
 * Retrieve the series ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_seriesid(cmyth_proginfo_t prog);

/**
 * Retrieve the program ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_programid(cmyth_proginfo_t prog);

/**
 * Retrieve the inetref of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_inetref(cmyth_proginfo_t prog);

/**
 * Retrieve the critics rating (number of stars) of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_stars(cmyth_proginfo_t prog);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_end(cmyth_proginfo_t prog);

/**
 * Retrieve the original air date of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_originalairdate(cmyth_proginfo_t prog);

/**
 * Retrieve the recording status of a program.
 * \param prog proginfo handle
 * \return recording status
 */
extern cmyth_proginfo_rec_status_t cmyth_proginfo_rec_status(
	cmyth_proginfo_t prog);

/**
 * Retrieve the flags associated with a program.
 * \param prog proginfo handle
 * \return flags
 */
extern unsigned long cmyth_proginfo_flags(
  cmyth_proginfo_t prog);

/**
 * Retrieve the size, in bytes, of a program.
 * \param prog proginfo handle
 * \return program length
 */
extern long long cmyth_proginfo_length(cmyth_proginfo_t prog);

/**
 * Retrieve the hostname of the MythTV backend that recorded a program.
 * \param prog proginfo handle
 * \return MythTV backend hostname
 */
extern char *cmyth_proginfo_host(cmyth_proginfo_t prog);

extern int cmyth_proginfo_port(cmyth_proginfo_t prog);

/**
 * Determine if two proginfo handles refer to the same program.
 * \param a proginfo handle a
 * \param b proginfo handle b
 * \retval 0 programs are the same
 * \retval -1 programs are different
 */
extern int cmyth_proginfo_compare(cmyth_proginfo_t a, cmyth_proginfo_t b);

/**
 * Retrieve the program length in seconds.
 * \param prog proginfo handle
 * \return program length in seconds
 */
extern int cmyth_proginfo_length_sec(cmyth_proginfo_t prog);

extern cmyth_proginfo_t cmyth_proginfo_get_detail(cmyth_conn_t control,
						  cmyth_proginfo_t p);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_end(cmyth_proginfo_t prog);

/**
 * Retrieve the card ID where the program was recorded.
 * \param prog proginfo handle
 * \return card ID
 */
extern long cmyth_proginfo_card_id(cmyth_proginfo_t prog);

/**
 * Retrieve the recording group of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_recgroup(cmyth_proginfo_t prog);

/**
 * Retrieve the channel icon path this program info
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanicon(cmyth_proginfo_t prog);


/**
 * Retrieve the production year for this program info
 * \param prog proginfo handle
 * \return production year
 */
extern unsigned short cmyth_proginfo_year(cmyth_proginfo_t prog);

/*
 * -----------------------------------------------------------------
 * Program List Operations
 * -----------------------------------------------------------------
 */

extern cmyth_proglist_t cmyth_proglist_create(void);

extern cmyth_proglist_t cmyth_proglist_get_all_recorded(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_all_pending(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_all_scheduled(cmyth_conn_t control);

extern cmyth_proglist_t cmyth_proglist_get_conflicting(cmyth_conn_t control);

extern cmyth_proginfo_t cmyth_proglist_get_item(cmyth_proglist_t pl,
						int index);

extern int cmyth_proglist_delete_item(cmyth_proglist_t pl,
				      cmyth_proginfo_t prog);

extern int cmyth_proglist_get_count(cmyth_proglist_t pl);

extern int cmyth_proglist_sort(cmyth_proglist_t pl, int count,
			       cmyth_proglist_sort_t sort);

/*
 * -----------------------------------------------------------------
 * File Transfer Operations
 * -----------------------------------------------------------------
 */
extern cmyth_conn_t cmyth_file_data(cmyth_file_t file);

extern unsigned long long cmyth_file_start(cmyth_file_t file);

extern unsigned long long cmyth_file_length(cmyth_file_t file);

extern int cmyth_file_get_block(cmyth_file_t file, char *buf,
				unsigned long len);

extern int cmyth_file_request_block(cmyth_file_t file, unsigned long len);

extern long long cmyth_file_seek(cmyth_file_t file,
				 long long offset, int whence);

extern int cmyth_file_select(cmyth_file_t file, struct timeval *timeout);

extern void cmyth_file_set_closed_callback(cmyth_file_t file,
					void (*callback)(cmyth_file_t));

extern int cmyth_file_read(cmyth_file_t file,
			   char *buf,
			   unsigned long len);
/*
 * -----------------------------------------------------------------
 * Channel Operations
 * -----------------------------------------------------------------
 */

extern long cmyth_channel_chanid(cmyth_channel_t channel);

extern long cmyth_channel_channum(cmyth_channel_t channel);

extern char * cmyth_channel_name(cmyth_channel_t channel);

extern char * cmyth_channel_icon(cmyth_channel_t channel);

extern int cmyth_channel_visible(cmyth_channel_t channel);

extern cmyth_channel_t cmyth_chanlist_get_item(cmyth_chanlist_t pl, int index);

extern int cmyth_chanlist_get_count(cmyth_chanlist_t pl);

/*
 * -----------------------------------------------------------------
 * Free Space Operations
 * -----------------------------------------------------------------
 */
extern cmyth_freespace_t cmyth_freespace_create(void);

/*
 * -------
 * Bookmark,Commercial Skip Operations
 * -------
 */
extern long long cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern int cmyth_get_bookmark_offset(cmyth_database_t db, long chanid, long long mark, char *starttime, int mode);
extern int cmyth_update_bookmark_setting(cmyth_database_t, cmyth_proginfo_t);
extern long long cmyth_get_bookmark_mark(cmyth_database_t, cmyth_proginfo_t, long long, int);
extern int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog,
	long long bookmark);
extern cmyth_commbreaklist_t cmyth_commbreaklist_create(void);
extern cmyth_commbreak_t cmyth_commbreak_create(void);
extern cmyth_commbreaklist_t cmyth_mysql_get_commbreaklist(cmyth_database_t db, cmyth_conn_t conn, cmyth_proginfo_t prog);
extern cmyth_commbreaklist_t cmyth_get_commbreaklist(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern cmyth_commbreaklist_t cmyth_get_cutlist(cmyth_conn_t conn, cmyth_proginfo_t prog);
extern int cmyth_rcv_commbreaklist(cmyth_conn_t conn, int *err, cmyth_commbreaklist_t breaklist, int count);

/*
 * mysql info
 */


typedef struct cmyth_program {
	int chanid;
	char callsign[30];
	char name[84];
	int sourceid;
	char title[150];
	char subtitle[150];
	char description[280];
	time_t starttime;
	time_t endtime;
	char programid[30];
	char seriesid[24];
	char category[84];
	int recording;
	int rec_status;
	int channum;
	int event_flags;
	int startoffset;
	int endoffset;
}cmyth_program_t;

typedef struct cmyth_recgrougs {
	char recgroups[33];
}cmyth_recgroups_t;

extern int cmyth_mysql_get_recgroups(cmyth_database_t, cmyth_recgroups_t **);
extern int cmyth_mysql_delete_scheduled_recording(cmyth_database_t db, char * query);
extern int cmyth_mysql_insert_into_record(cmyth_database_t db, char * query, char * query1, char * query2, char *title, char * subtitle, char * description, char * callsign);

extern char* cmyth_get_recordid_mysql(cmyth_database_t, int, char *, char *, char *, char *, char *);
extern int cmyth_get_offset_mysql(cmyth_database_t, int, char *, int, char *, char *, char *, char *, char *);

extern int cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, char *program_name);
extern int cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_program_t **prog,  time_t starttime, char *program_name);
extern int cmyth_mysql_get_guide(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, time_t endtime);
extern int cmyth_mysql_testdb_connection(cmyth_database_t db,char **message);
extern int cmyth_schedule_recording(cmyth_conn_t conn, char * msg);
extern char * cmyth_mysql_escape_chars(cmyth_database_t db, char * string);
extern int cmyth_mysql_get_commbreak_list(cmyth_database_t db, int chanid, char * start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version);

extern int cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_program_t **prog);

extern int cmyth_get_delete_list(cmyth_conn_t, char *, cmyth_proglist_t);

#define PROGRAM_ADJUST  3600

extern int cmyth_mythtv_remove_previous_recorded(cmyth_database_t db,char *query);

extern cmyth_chanlist_t cmyth_mysql_get_chanlist(cmyth_database_t db);
#endif /* __CMYTH_H */
