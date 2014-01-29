/*
 * Copyright (C) 2001-2004 Rich Wareham <richwareham@users.sourceforge.net>
 *
 * This file is part of libdvdnav, a DVD navigation library.
 *
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBDVDNAV_DVDNAV_INTERNAL_H
#define LIBDVDNAV_DVDNAV_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32

/* pthread_mutex_* wrapper for win32 */
#include <windows.h>
#include <process.h>
typedef CRITICAL_SECTION pthread_mutex_t;
#define pthread_mutex_init(a, b) InitializeCriticalSection(a)
#define pthread_mutex_lock(a)    EnterCriticalSection(a)
#define pthread_mutex_unlock(a)  LeaveCriticalSection(a)
#define pthread_mutex_destroy(a) DeleteCriticalSection(a)

#ifndef HAVE_GETTIMEOFDAY
/* replacement gettimeofday implementation */
#include <sys/timeb.h>
static inline int _private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#define gettimeofday(TV, TZ) _private_gettimeofday((TV), (TZ))
#endif

#include <io.h> /* read() */
#define lseek64 _lseeki64

#else

#include <pthread.h>

#endif /* WIN32 */

/* where should libdvdnav write its messages (stdout/stderr) */
#define MSG_OUT stderr

/* Maximum length of an error string */
#define MAX_ERR_LEN 255

/* Use the POSIX PATH_MAX if available */
#ifdef PATH_MAX
#define MAX_PATH_LEN PATH_MAX
#else
#define MAX_PATH_LEN 255 /* Arbitrary */
#endif

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

typedef enum {
  DSI_ILVU_PRE   = 1 << 15, /* set during the last 3 VOBU preceeding an interleaved block. */
  DSI_ILVU_BLOCK = 1 << 14, /* set for all VOBU in an interleaved block */
  DSI_ILVU_FIRST = 1 << 13, /* set for the first VOBU for a given angle or scene within a ILVU, or the first VOBU in the preparation (PREU) sequence */
  DSI_ILVU_LAST  = 1 << 12, /* set for the last VOBU for a given angle or scene within a ILVU, or the last VOBU in the preparation (PREU) sequence */
  DSI_ILVU_MASK  = 0xf000
} DSI_ILVU;

typedef struct read_cache_s read_cache_t;

/*
 * These are defined here because they are
 * not in ifo_types.h, they maybe one day
 */

#ifndef audio_status_t
typedef struct {
#ifdef WORDS_BIGENDIAN
  unsigned int available     : 1;
  unsigned int zero1         : 4;
  unsigned int stream_number : 3;
  uint8_t zero2;
#else
  uint8_t zero2;
  unsigned int stream_number : 3;
  unsigned int zero1         : 4;
  unsigned int available     : 1;
#endif
} ATTRIBUTE_PACKED audio_status_t;
#endif

#ifndef spu_status_t
typedef struct {
#ifdef WORDS_BIGENDIAN
  unsigned int available               : 1;
  unsigned int zero1                   : 2;
  unsigned int stream_number_4_3       : 5;
  unsigned int zero2                   : 3;
  unsigned int stream_number_wide      : 5;
  unsigned int zero3                   : 3;
  unsigned int stream_number_letterbox : 5;
  unsigned int zero4                   : 3;
  unsigned int stream_number_pan_scan  : 5;
#else
  unsigned int stream_number_pan_scan  : 5;
  unsigned int zero4                   : 3;
  unsigned int stream_number_letterbox : 5;
  unsigned int zero3                   : 3;
  unsigned int stream_number_wide      : 5;
  unsigned int zero2                   : 3;
  unsigned int stream_number_4_3       : 5;
  unsigned int zero1                   : 2;
  unsigned int available               : 1;
#endif
} ATTRIBUTE_PACKED spu_status_t;
#endif

/*
 * Describes a given time, and the closest sector, vobu and tmap index
 */
typedef struct {
  uint64_t            time;
  uint32_t            sector;
  uint32_t            vobu_idx;
  int32_t             tmap_idx;
} dvdnav_pos_data_t;

/*
 * Encapsulates cell data
 */
typedef struct {
  int32_t             idx;
  dvdnav_pos_data_t   *bgn;
  dvdnav_pos_data_t   *end;
} dvdnav_cell_data_t;

/*
 * Encapsulates common variables used by internal functions of jump_to_time
 */
typedef struct {
  vobu_admap_t        *admap;
  int32_t             admap_len;
  vts_tmap_t          *tmap;
  int32_t             tmap_len;
  int32_t             tmap_interval;
} dvdnav_jump_args_t;

/*
 * Utility constants for jump_to_time
 */
#define TMAP_IDX_EDGE_BGN  -1
#define TMAP_IDX_EDGE_END  -2
#define JUMP_MODE_TIME_AFTER 1
#define JUMP_MODE_TIME_DEFAULT 0
#define JUMP_MODE_TIME_BEFORE -1

typedef struct dvdnav_vobu_s {
  int32_t vobu_start;  /* Logical Absolute. MAX needed is 0x300000 */
  int32_t vobu_length;
  int32_t blockN;      /* Relative offset */
  int32_t vobu_next;   /* Relative offset */
} dvdnav_vobu_t;

/** The main DVDNAV type **/

struct dvdnav_s {
  /* General data */
  char        path[MAX_PATH_LEN]; /* Path to DVD device/dir */
  dvd_file_t *file;               /* Currently opened file */

  /* Position data */
  vm_position_t position_next;
  vm_position_t position_current;
  dvdnav_vobu_t vobu;

  /* NAV data */
  pci_t pci;
  dsi_t dsi;
  uint32_t last_cmd_nav_lbn;      /* detects when a command is issued on an already left NAV */

  /* Flags */
  int skip_still;                 /* Set when skipping a still */
  int sync_wait;                  /* applications should wait till they are in sync with us */
  int sync_wait_skip;             /* Set when skipping wait state */
  int spu_clut_changed;           /* The SPU CLUT changed */
  int started;                    /* vm_start has been called? */
  int use_read_ahead;             /* 1 - use read-ahead cache, 0 - don't */
  int pgc_based;                  /* positioning works PGC based instead of PG based */
  int cur_cell_time;              /* time expired since the beginning of the current cell, read from the dsi */

  /* VM */
  vm_t *vm;
  pthread_mutex_t vm_lock;

  /* Read-ahead cache */
  read_cache_t *cache;

  /* Errors */
  char err_str[MAX_ERR_LEN];
};

/** HELPER FUNCTIONS **/

/* converts a dvd_time_t to PTS ticks */
int64_t dvdnav_convert_time(dvd_time_t *time);

/* XBMC added functions */
/*
 * Get current playback state
 */
dvdnav_status_t dvdnav_get_state(dvdnav_t *this, dvd_state_t *save_state);

/*
 * Resume playback state
 */
dvdnav_status_t dvdnav_set_state(dvdnav_t *this, dvd_state_t *save_state);
/* end XBMC */

/** USEFUL MACROS **/

#ifdef __GNUC__
#define printerrf(format, args...) \
	do { if (this) snprintf(this->err_str, MAX_ERR_LEN, format, ## args); } while (0)
#else
#ifdef _MSC_VER
#define printerrf(str) \
	do { if (this) snprintf(this->err_str, MAX_ERR_LEN, str); } while (0)
#else
#define printerrf(...) \
	do { if (this) snprintf(this->err_str, MAX_ERR_LEN, __VA_ARGS__); } while (0)
#endif /* WIN32 */
#endif
#define printerr(str) \
	do { if (this) strncpy(this->err_str, str, MAX_ERR_LEN - 1); } while (0)

#endif /* LIBDVDNAV_DVDNAV_INTERNAL_H */
