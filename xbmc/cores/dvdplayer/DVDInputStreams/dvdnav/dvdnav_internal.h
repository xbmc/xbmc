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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 */

#ifndef DVDNAV_INTERNAL_H_INCLUDED
#define DVDNAV_INTERNAL_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <limits.h>
#include <string.h>

#ifdef WIN32

/* pthread_mutex_* wrapper for win32 */
#ifndef _LINUX
#include <process.h>
typedef CRITICAL_SECTION pthread_mutex_t;
#define pthread_mutex_init(a, b) InitializeCriticalSection(a)
#define pthread_mutex_lock(a)    EnterCriticalSection(a)
#define pthread_mutex_unlock(a)  LeaveCriticalSection(a)
#define pthread_mutex_destroy(a)
#endif

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
#ifndef _LINUX
#include <io.h> /* read() */
#endif
#define lseek64 _lseeki64

#else

#include <pthread.h>

#endif /* WIN32 */

/* Uncomment for VM command tracing */
/* #define TRACE */

#include "decoder.h"
#include "dvdnav.h"
#include "vm.h"
#include "vmcmd.h"

/* where should libdvdnav write its messages (stdout/stderr) */
#define MSG_OUT stdout

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
  
  /* VM */
  vm_t *vm;
  pthread_mutex_t vm_lock;

  /* Read-ahead cache */
  read_cache_t *cache;

  /* Errors */
  char err_str[MAX_ERR_LEN];
};

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

#endif /* DVDNAV_INTERNAL_H_INCLUDED */
