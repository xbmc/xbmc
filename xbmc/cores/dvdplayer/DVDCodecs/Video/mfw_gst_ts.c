/*
 * Copyright (c) 2010-2012, Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Module Name:    TimeStamp.c
 *
 * Description:    include TimeStamp stratege for VPU / SW video decoder plugin
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */

/*
 * Changelog:
  11/2/2010        draft version       Lyon Wang
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mfw_gst_ts.h"


const char *debug_env = "ME_DEBUG";
char *debug = NULL;
int debug_level = 0;


enum
{
  DEBUG_LEVEL_ERROR = 1,
  DEBUG_LEVEL_WARNING,
  DEBUG_LEVEL_LOG,
  DEBUG_LEVEL_VERBOSE,
};


#define TSM_MESSAGE(level, fmt, ...)\
  do{\
    if (debug_level>=(level)){\
      printf("TSM:"fmt, ##__VA_ARGS__);\
    }\
  }while(0)

#define TSM_ERROR(...) TSM_MESSAGE(DEBUG_LEVEL_ERROR, ##__VA_ARGS__)
#define TSM_WARNING(...) TSM_MESSAGE(DEBUG_LEVEL_WARNING, ##__VA_ARGS__)
#define TSM_LOG(...) TSM_MESSAGE(DEBUG_LEVEL_LOG, ##__VA_ARGS__)
#define TSM_VERBOSE(...) TSM_MESSAGE(DEBUG_LEVEL_VERBOSE, ##__VA_ARGS__)

#define TSM_HISTORY_POWER 5
#define TSM_HISTORY_SIZE (1<<TSM_HISTORY_POWER)
#define TSM_ADAPTIVE_INTERVAL(tsm) \
    (tsm->dur_history_total>>TSM_HISTORY_POWER)

#define TSM_SECOND ((TSM_TIMESTAMP)1000000000)
#define TSM_DEFAULT_INTERVAL (TSM_SECOND/30)
#define TSM_DEFAULT_TS_BUFFER_SIZE (128)

#define TSM_TS_IS_VALID(ts)	\
    ((ts) != TSM_TIMESTAMP_NONE)

#define TSM_KEY_IS_VALID(key) \
    ((key) != TSM_KEY_NONE)

#define TSM_DISTANCE(tsm)\
    (((tsm->rx)>=(tsm->tx))?((tsm->rx)-(tsm->tx)):(tsm->ts_buf_size-(tsm->tx)+(tsm->rx)))

#define TSM_PLUS_AGE(tsm)\
    (TSM_DISTANCE(tsm)+tsm->invalid_ts_count+2)

#define TSM_ABS(ts0, ts1)\
    (((ts0)>(ts1))?((ts0)-(ts1)):((ts1)-(ts0)))

#define TSM_TIME_FORMAT "u:%02u:%02u.%09u"

#define TSM_TIME_ARGS(t) \
        TSM_TS_IS_VALID (t) ? \
        (unsigned int) (((TSM_TIMESTAMP)(t)) / (TSM_SECOND * 60 * 60)) : 99, \
        TSM_TS_IS_VALID (t) ? \
        (unsigned int) ((((TSM_TIMESTAMP)(t)) / (TSM_SECOND * 60)) % 60) : 99, \
        TSM_TS_IS_VALID (t) ? \
        (unsigned int) ((((TSM_TIMESTAMP)(t)) / TSM_SECOND) % 60) : 99, \
        TSM_TS_IS_VALID (t) ? \
        (unsigned int) (((TSM_TIMESTAMP)(t)) % TSM_SECOND) : 999999999

#define TSM_BUFFER_SET(buf, value, size) \
    do {\
        int i;\
        for (i=0;i<(size);i++){\
            (buf)[i] = (value);\
        }\
    }while(0)

#define TSM_RECEIVED_NUNBER 512


typedef struct
{
  TSM_TIMESTAMP ts;
  unsigned long long age;
  void *key;
} TSMControl;

typedef struct _TSMReceivedEntry
{
  TSM_TIMESTAMP ts;
  struct _TSMReceivedEntry *next;
  unsigned int used:1;
  unsigned int subentry:1;
  int size;
} TSMReceivedEntry;

typedef struct _TSMReceivedEntryMemory
{
  struct _TSMReceivedEntryMemory *next;
  TSMReceivedEntry entrys[TSM_RECEIVED_NUNBER];
} TSMReceivedEntryMemory;

typedef struct
{
  TSMReceivedEntry *head;
  TSMReceivedEntry *tail;
  TSMReceivedEntry *free;
  TSMReceivedEntryMemory *memory;
  int cnt;
} TSMRecivedCtl;

typedef struct _TSManager
{
  int first_tx;
  int first_rx;
  int rx;                       //timestamps received
  int tx;                       //timestamps transfered
  TSM_TIMESTAMP last_ts_sent;   //last time stamp sent
  TSM_TIMESTAMP last_ts_received;
  TSM_TIMESTAMP suspicious_ts;

  TSM_TIMESTAMP discont_threshold;

  unsigned int invalid_ts_count;
  TSMGR_MODE mode;
  int ts_buf_size;
  int dur_history_tx;
  TSM_TIMESTAMP dur_history_total;
  TSM_TIMESTAMP dur_history_buf[TSM_HISTORY_SIZE];
  TSMControl *ts_buf;
  unsigned long long age;
  int tx_cnt;
  int rx_cnt;
  int cnt;
  int valid_ts_received:1;
  int big_cnt;

  TSMRecivedCtl rctl;
} TSManager;


static void
tsm_free_received_entry (TSMRecivedCtl * rctl, TSMReceivedEntry * entry)
{
  entry->next = rctl->free;
  rctl->free = entry;
}


static TSMReceivedEntry *
tsm_new_received_entry (TSMRecivedCtl * rctl)
{
  TSMReceivedEntry *ret = NULL;
  if (rctl->free) {
    ret = rctl->free;
    rctl->free = ret->next;
  } else {
    TSMReceivedEntryMemory *p = malloc (sizeof (TSMReceivedEntryMemory));
    if (p) {
      int i;
      for (i = 1; i < TSM_RECEIVED_NUNBER; i++) {
        TSMReceivedEntry *e = &p->entrys[i];
        tsm_free_received_entry (rctl, e);
      };

      p->next = rctl->memory;
      rctl->memory = p;

      ret = p->entrys;
    }
  }
  return ret;
}


void
TSManagerReceive2 (void *handle, TSM_TIMESTAMP timestamp, int size)
{
#define CLEAR_TSM_RENTRY(entry)\
  do { \
    (entry)->used = 0; \
    (entry)->subentry = 0; \
    (entry)->next = NULL; \
  } while (0)
  TSManager *tsm = (TSManager *) handle;

  TSM_VERBOSE ("receive2 %" TSM_TIME_FORMAT " size %d\n",
      TSM_TIME_ARGS (timestamp), size);

  if (tsm) {
    if (size > 0) {
      TSMRecivedCtl *rctl = &tsm->rctl;
      TSMReceivedEntry *e = tsm_new_received_entry (rctl);
      if (e) {
        CLEAR_TSM_RENTRY (e);
        if ((rctl->tail) && (rctl->tail->ts == timestamp)) {
          e->subentry = 1;
        }
        e->ts = timestamp;
        e->size = size;
        if (rctl->tail) {
          rctl->tail->next = e;
          rctl->tail = e;
        } else {
          rctl->head = rctl->tail = e;
        }
      }
      rctl->cnt++;
    } else {
      TSManagerReceive (handle, timestamp);
    }
  }
}


static TSM_TIMESTAMP
TSManagerGetLastTimeStamp (TSMRecivedCtl * rctl, int size, int use)
{
  TSM_TIMESTAMP ts = TSM_TIMESTAMP_NONE;
  TSMReceivedEntry *e;
  while ((size > 0) && (e = rctl->head)) {
    ts = ((e->used) ? (TSM_TIMESTAMP_NONE) : (e->ts));
    if (use)
      e->used = 1;
    if (size >= e->size) {
      rctl->head = e->next;
      if (rctl->head == NULL) {
        rctl->tail = NULL;
      } else {
        if (rctl->head->subentry) {
          rctl->head->used = e->used;
        }
      }
      size -= e->size;
      rctl->cnt--;
      tsm_free_received_entry (rctl, e);
    } else {
      e->size -= size;
      size = 0;
    }
  }
  return ts;
}


void
TSManagerFlush2 (void *handle, int size)
{
  TSManager *tsm = (TSManager *) handle;
  if (tsm) {
    TSManagerGetLastTimeStamp (&tsm->rctl, size, 0);
  }

}


/*======================================================================================
FUNCTION:           mfw_gst_receive_ts

DESCRIPTION:        Check timestamp and do frame dropping if enabled

ARGUMENTS PASSED:   pTimeStamp_Object  - TimeStamp Manager to handle related timestamp
                    timestamp - time stamp of the input buffer which has video data.

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=======================================================================================*/
static void
_TSManagerReceive (void *handle, TSM_TIMESTAMP timestamp, void *key)
{
  TSManager *tsm = (TSManager *) handle;

  if (tsm) {
    if (TSM_TS_IS_VALID (timestamp) && (tsm->rx_cnt))
      tsm->valid_ts_received = 1;
    tsm->rx_cnt++;
    if (tsm->cnt < tsm->ts_buf_size - 1) {
      tsm->cnt++;
      if (tsm->mode == MODE_AI) {

        if (TSM_TS_IS_VALID (timestamp)) {
          if (tsm->first_rx) {
            tsm->last_ts_received = timestamp;
            tsm->first_rx = 0;
          } else {
            if (tsm->suspicious_ts) {
              if (timestamp >= tsm->suspicious_ts) {
                tsm->last_ts_received = timestamp;
              }
              tsm->suspicious_ts = 0;
            }
            if ((timestamp > tsm->last_ts_received)
                && (timestamp - tsm->last_ts_received > tsm->discont_threshold)) {
              tsm->suspicious_ts = timestamp;
              timestamp = TSM_TIMESTAMP_NONE;
            }
          }
        }

        if (TSM_TS_IS_VALID (timestamp))        // && (TSM_ABS(timestamp, tsm->last_ts_sent)<TSM_SECOND*10))
        {
          tsm->ts_buf[tsm->rx].ts = timestamp;
          tsm->ts_buf[tsm->rx].age = tsm->age + TSM_PLUS_AGE (tsm);
          tsm->ts_buf[tsm->rx].key = key;
          tsm->last_ts_received = timestamp;
#ifdef DEBUG
          //printf("age should %lld %lld\n", tsm->age, tsm->ts_buf[tsm->rx].age);
          //printf("++++++ distance = %d  tx=%d, rx=%d, invalid count=%d\n", TSM_DISTANCE(tsm), tsm->tx, tsm->rx,tsm->invalid_ts_count);
#endif
          tsm->rx = ((tsm->rx + 1) % tsm->ts_buf_size);
        } else {
          tsm->invalid_ts_count++;
        }
      } else if (tsm->mode == MODE_FIFO) {
        tsm->ts_buf[tsm->rx].ts = timestamp;
        tsm->rx = ((tsm->rx + 1) % tsm->ts_buf_size);
      }
      TSM_LOG ("++Receive %d:%" TSM_TIME_FORMAT
          ", invalid:%d, size:%d key %p\n", tsm->rx_cnt,
          TSM_TIME_ARGS (timestamp), tsm->invalid_ts_count, tsm->cnt, key);
    } else {
      TSM_ERROR ("Too many timestamps recieved!! (cnt=%d)\n", tsm->cnt);
    }
  }
}


void
TSManagerValid2 (void *handle, int size, void *key)
{
  TSManager *tsm = (TSManager *) handle;

  TSM_VERBOSE ("valid2 size %d\n", size);

  if (tsm) {
    TSM_TIMESTAMP ts;
    ts = TSManagerGetLastTimeStamp (&tsm->rctl, size, 1);
    _TSManagerReceive (tsm, ts, key);
  }
}


void
TSManagerReceive (void *handle, TSM_TIMESTAMP timestamp)
{
  _TSManagerReceive (handle, timestamp, TSM_KEY_NONE);
}


/*======================================================================================
FUNCTION:           TSManagerSend

DESCRIPTION:        Check timestamp and do frame dropping if enabled

ARGUMENTS PASSED:   pTimeStamp_Object  - TimeStamp Manager to handle related timestamp
                    ptimestamp - returned timestamp to use at render

RETURN VALUE:       None
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=======================================================================================*/
static TSM_TIMESTAMP
_TSManagerSend2 (void *handle, void *key, int send)
{
  TSManager *tsm = (TSManager *) handle;
  int i = tsm->tx;
  int index = -1;
  TSM_TIMESTAMP ts0 = 0, tstmp = TSM_TIMESTAMP_NONE;
  unsigned long long age = 0;
  TSM_TIMESTAMP half_interval = TSM_ADAPTIVE_INTERVAL (tsm) >> 1;

  if (tsm) {
    if (send) {
      tsm->tx_cnt++;
    } else {
      tsm->cnt++;
      tsm->invalid_ts_count++;
    }
    if (tsm->cnt > 0) {
      if (send) {
        tsm->cnt--;
      }
      if (tsm->mode == MODE_AI) {

        if (tsm->first_tx == 0) {
          tstmp = tsm->last_ts_sent + TSM_ADAPTIVE_INTERVAL (tsm);
        } else {
          tstmp = tsm->last_ts_sent;
        }

        while (i != tsm->rx) {
          if (index >= 0) {
            if (tsm->ts_buf[i].ts < ts0) {
              ts0 = tsm->ts_buf[i].ts;
              age = tsm->ts_buf[i].age;
              index = i;
            }
          } else {
            ts0 = tsm->ts_buf[i].ts;
            age = tsm->ts_buf[i].age;
            index = i;
          }
          if ((TSM_KEY_IS_VALID (key)) && (key == tsm->ts_buf[i].key))
            break;
          i = ((i + 1) % tsm->ts_buf_size);
        }
        if (index >= 0) {
          if ((tsm->invalid_ts_count) && (ts0 >= ((tstmp) + half_interval))
              && (age > tsm->age)) {
            /* use calculated ts0 */
            if (send) {
              tsm->invalid_ts_count--;
            }
          } else {

            if (send) {
              if (index != tsm->tx) {
                tsm->ts_buf[index] = tsm->ts_buf[tsm->tx];
              }
              tsm->tx = ((tsm->tx + 1) % tsm->ts_buf_size);

            }
#if 0
            if (ts0 >= ((tstmp) + half_interval))
              tstmp = tstmp;
            else
              tstmp = ts0;
#else
            tstmp = ts0;
#endif
          }

        } else {
          if (send) {
            tsm->invalid_ts_count--;
          }
        }

        if (tsm->first_tx == 0) {

          if (tstmp > tsm->last_ts_sent) {
            ts0 = (tstmp - tsm->last_ts_sent);
          } else {
            ts0 = 0;
            tstmp = tsm->last_ts_sent;
          }

          if (ts0 > TSM_ADAPTIVE_INTERVAL (tsm) * 3 / 2) {
            TSM_WARNING ("Jitter1:%" TSM_TIME_FORMAT " %" TSM_TIME_FORMAT "\n",
                TSM_TIME_ARGS (ts0),
                TSM_TIME_ARGS (TSM_ADAPTIVE_INTERVAL (tsm) * 3 / 2));
          } else if (ts0 == 0) {
            TSM_WARNING ("Jitter:%" TSM_TIME_FORMAT "\n", TSM_TIME_ARGS (ts0));
          }

          if (send) {
            if ((ts0 < TSM_ADAPTIVE_INTERVAL (tsm) * 2) || (tsm->big_cnt > 3)) {
              tsm->big_cnt = 0;
              tsm->dur_history_total -=
                  tsm->dur_history_buf[tsm->dur_history_tx];
              tsm->dur_history_buf[tsm->dur_history_tx] = ts0;
              tsm->dur_history_tx =
                  ((tsm->dur_history_tx + 1) % TSM_HISTORY_SIZE);
              tsm->dur_history_total += ts0;
            } else {
              tsm->big_cnt++;
            }
          }
        }

        if (send) {
          tsm->last_ts_sent = tstmp;
          tsm->age++;
          tsm->first_tx = 0;
        }

      } else if (tsm->mode == MODE_FIFO) {
        tstmp = tsm->ts_buf[tsm->tx].ts;
        if (send) {
          tsm->tx = ((tsm->tx + 1) % tsm->ts_buf_size);
        }
        ts0 = tstmp - tsm->last_ts_sent;
        if (send) {
          tsm->last_ts_sent = tstmp;
        }
      }

      if (send) {
        TSM_LOG ("--Send %d:%" TSM_TIME_FORMAT ", int:%" TSM_TIME_FORMAT
            ", avg:%" TSM_TIME_FORMAT " inkey %p\n", tsm->tx_cnt,
            TSM_TIME_ARGS (tstmp), TSM_TIME_ARGS (ts0),
            TSM_TIME_ARGS (TSM_ADAPTIVE_INTERVAL (tsm)), key);
      }

    } else {
      if (tsm->valid_ts_received == 0) {
        if (tsm->first_tx) {
          tstmp = tsm->last_ts_sent;
        } else {
          tstmp = tsm->last_ts_sent + TSM_ADAPTIVE_INTERVAL (tsm);
        }
        if (send) {
          tsm->first_tx = 0;
          tsm->last_ts_sent = tstmp;
        }
      }
      TSM_ERROR ("Too many timestamps send!!\n");
    }

    if (send == 0) {
      tsm->cnt--;
      tsm->invalid_ts_count--;
    }

  }

  return tstmp;
}


TSM_TIMESTAMP
TSManagerSend2 (void *handle, void *key)
{
  return _TSManagerSend2 (handle, key, 1);
}


TSM_TIMESTAMP
TSManagerQuery2 (void *handle, void *key)
{
  return _TSManagerSend2 (handle, key, 0);
}


TSM_TIMESTAMP
TSManagerSend (void *handle)
{
  return TSManagerSend2 (handle, TSM_KEY_NONE);
}


TSM_TIMESTAMP
TSManagerQuery (void *handle)
{
  return TSManagerQuery2 (handle, TSM_KEY_NONE);
}


void
resyncTSManager (void *handle, TSM_TIMESTAMP synctime, TSMGR_MODE mode)
{
  TSManager *tsm = (TSManager *) handle;
  if (tsm) {
    TSMRecivedCtl *rctl = &tsm->rctl;
    TSMReceivedEntry *e = rctl->head;

    while ((e = rctl->head)) {
      rctl->head = e->next;
      tsm_free_received_entry (rctl, e);
    };
    rctl->cnt = 0;

    rctl->tail = NULL;

    tsm->first_tx = 1;
    tsm->first_rx = 1;
    tsm->suspicious_ts = 0;

    if (TSM_TS_IS_VALID (synctime))
      tsm->last_ts_sent = synctime;

    tsm->tx = tsm->rx = 0;
    tsm->invalid_ts_count = 0;
    tsm->mode = mode;
    tsm->age = 0;
    tsm->rx_cnt = tsm->tx_cnt = tsm->cnt = 0;
    tsm->valid_ts_received = 0;

    tsm->big_cnt = 0;
  }
}


/*======================================================================================
FUNCTION:           mfw_gst_init_ts

DESCRIPTION:        malloc and initialize timestamp strcture

ARGUMENTS PASSED:   ppTimeStamp_Object  - pointer of TimeStamp Manager to handle related timestamp

RETURN VALUE:       TimeStamp structure pointer
PRE-CONDITIONS:     None
POST-CONDITIONS:    None
IMPORTANT NOTES:    None
=======================================================================================*/
void *
createTSManager (int ts_buf_size)
{
  TSManager *tsm = (TSManager *) malloc (sizeof (TSManager));
  debug = getenv (debug_env);
  if (debug) {
    debug_level = atoi (debug);
  }
  // printf("debug = %s \n ++++++++++++++++++++++++++++",debug);
  if (tsm) {
    memset (tsm, 0, sizeof (TSManager));
    if (ts_buf_size <= 0) {
      ts_buf_size = TSM_DEFAULT_TS_BUFFER_SIZE;
    }
    tsm->ts_buf_size = ts_buf_size;
    tsm->ts_buf = malloc (sizeof (TSMControl) * ts_buf_size);

    if (tsm->ts_buf == NULL) {
      goto fail;
    }

    resyncTSManager (tsm, (TSM_TIMESTAMP) 0, MODE_AI);

    tsm->dur_history_tx = 0;
    TSM_BUFFER_SET (tsm->dur_history_buf, TSM_DEFAULT_INTERVAL,
        TSM_HISTORY_SIZE);
    tsm->dur_history_total = TSM_DEFAULT_INTERVAL << TSM_HISTORY_POWER;

    tsm->discont_threshold = 10000000000LL;     // 10s
  }
  return tsm;
fail:
  if (tsm) {
    if (tsm->ts_buf) {
      free (tsm->ts_buf);
    }
    free (tsm);
    tsm = NULL;
  }
  return tsm;
}


void
destroyTSManager (void *handle)
{
  TSManager *tsm = (TSManager *) handle;
  if (tsm) {
    TSMRecivedCtl *rctl = &tsm->rctl;
    TSMReceivedEntryMemory *rmem;
    if (tsm->ts_buf) {
      free (tsm->ts_buf);
    }

    while ((rmem = rctl->memory)) {
      rctl->memory = rmem->next;
      free (rmem);
    }
    free (tsm);
    tsm = NULL;
  }
}


void
setTSManagerFrameRate (void *handle, int fps_n, int fps_d)
//void setTSManagerFrameRate(void * handle, float framerate)
{
  TSManager *tsm = (TSManager *) handle;
  TSM_TIMESTAMP ts;
  if ((fps_n > 0) && (fps_d > 0) && (fps_n / fps_d <= 80))
    ts = TSM_SECOND * fps_d / fps_n;
  else
    ts = TSM_DEFAULT_INTERVAL;
  // TSM_TIMESTAMP ts = TSM_SECOND / framerate;

  if (tsm) {
    TSM_BUFFER_SET (tsm->dur_history_buf, ts, TSM_HISTORY_SIZE);
    tsm->dur_history_total = (ts << TSM_HISTORY_POWER);
    if (debug)
      TSM_LOG ("Set frame intrval:%" TSM_TIME_FORMAT "\n", TSM_TIME_ARGS (ts));
  }
}


TSM_TIMESTAMP
getTSManagerFrameInterval (void *handle)
{
  TSManager *tsm = (TSManager *) handle;
  TSM_TIMESTAMP ts = 0;
  if (tsm) {
    ts = TSM_ADAPTIVE_INTERVAL (tsm);
  }
  return ts;
}


TSM_TIMESTAMP
getTSManagerPosition (void *handle)
{
  TSManager *tsm = (TSManager *) handle;
  TSM_TIMESTAMP ts = 0;
  if (tsm) {
    ts = tsm->last_ts_sent;
  }
  return ts;
}


int
getTSManagerPreBufferCnt (void *handle)
{
  int i = 0;
  TSManager *tsm = (TSManager *) handle;
  if (tsm) {
    i = tsm->rctl.cnt;
  }
  return i;
}
