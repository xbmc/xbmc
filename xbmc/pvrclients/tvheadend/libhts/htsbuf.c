/*
 *  Buffer management functions
 *  Copyright (C) 2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "htsbuf.h"
#ifdef _MSC_VER
#include "msvc.h"
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif


/**
 *
 */
void
htsbuf_queue_init(htsbuf_queue_t *hq, unsigned int maxsize)
{
  if(maxsize == 0)
    maxsize = INT32_MAX;
  TAILQ_INIT(&hq->hq_q);
  hq->hq_size = 0;
  hq->hq_maxsize = maxsize;
}


/**
 *
 */
void
htsbuf_data_free(htsbuf_queue_t *hq, htsbuf_data_t *hd)
{
  TAILQ_REMOVE(&hq->hq_q, hd, hd_link);
  free(hd->hd_data);
  free(hd);
}


/**
 *
 */
void
htsbuf_queue_flush(htsbuf_queue_t *hq)
{
  htsbuf_data_t *hd;

  hq->hq_size = 0;

  while((hd = TAILQ_FIRST(&hq->hq_q)) != NULL)
    htsbuf_data_free(hq, hd);
}

/**
 *
 */
void
htsbuf_append(htsbuf_queue_t *hq, const char *buf, size_t len)
{
  htsbuf_data_t *hd = TAILQ_LAST(&hq->hq_q, htsbuf_data_queue);
  int c;
  hq->hq_size += len;

  if(hd != NULL) {
    /* Fill out any previous buffer */
    c = MIN(hd->hd_data_size - hd->hd_data_len, len);
    memcpy(hd->hd_data + hd->hd_data_len, buf, c);
    hd->hd_data_len += c;
    buf += c;
    len -= c;
  }
  if(len == 0)
    return;
  
  hd = malloc(sizeof(htsbuf_data_t));
  TAILQ_INSERT_TAIL(&hq->hq_q, hd, hd_link);
  
  c = MAX(len, 1000); /* Allocate 1000 bytes to support lots of small writes */

  hd->hd_data = malloc(c);
  hd->hd_data_size = c;
  hd->hd_data_len = len;
  hd->hd_data_off = 0;
  memcpy(hd->hd_data, buf, len);
}

/**
 *
 */
void
htsbuf_append_prealloc(htsbuf_queue_t *hq, const char *buf, size_t len)
{
  htsbuf_data_t *hd;

  hq->hq_size += len;

  hd = malloc(sizeof(htsbuf_data_t));
  TAILQ_INSERT_TAIL(&hq->hq_q, hd, hd_link);
  
  hd->hd_data = (void *)buf;
  hd->hd_data_size = len;
  hd->hd_data_len = len;
  hd->hd_data_off = 0;
}

/**
 *
 */
size_t
htsbuf_read(htsbuf_queue_t *hq, char *buf, size_t len)
{
  size_t r = 0;
  int c;

  htsbuf_data_t *hd;
  
  while(len > 0) {
    hd = TAILQ_FIRST(&hq->hq_q);
    if(hd == NULL)
      break;

    c = MIN(hd->hd_data_len - hd->hd_data_off, len);
    memcpy(buf, hd->hd_data + hd->hd_data_off, c);

    r += c;
    buf += c;
    len -= c;
    hd->hd_data_off += c;
    hq->hq_size -= c;
    if(hd->hd_data_off == hd->hd_data_len)
      htsbuf_data_free(hq, hd);
  }
  return r;
}


/**
 *
 */
size_t
htsbuf_find(htsbuf_queue_t *hq, uint8_t v)
{
  htsbuf_data_t *hd;
  unsigned int i, o = 0;

  TAILQ_FOREACH(hd, &hq->hq_q, hd_link) {
    for(i = hd->hd_data_off; i < hd->hd_data_len; i++) {
      if(hd->hd_data[i] == v) 
	return o + i - hd->hd_data_off;
    }
    o += hd->hd_data_len - hd->hd_data_off;
  }
  return -1;
}



/**
 *
 */
size_t
htsbuf_peek(htsbuf_queue_t *hq, char *buf, size_t len)
{
  size_t r = 0;
  int c;

  htsbuf_data_t *hd = TAILQ_FIRST(&hq->hq_q);
  
  while(len > 0 && hd != NULL) {
    c = MIN(hd->hd_data_len - hd->hd_data_off, len);
    memcpy(buf, hd->hd_data + hd->hd_data_off, c);

    buf += c;
    len -= c;

    hd = TAILQ_NEXT(hd, hd_link);
  }
  return r;
}

/**
 *
 */
size_t
htsbuf_drop(htsbuf_queue_t *hq, size_t len)
{
  size_t r = 0;
  int c;
  htsbuf_data_t *hd;
  
  while(len > 0) {
    hd = TAILQ_FIRST(&hq->hq_q);
    if(hd == NULL)
      break;

    c = MIN(hd->hd_data_len - hd->hd_data_off, len);
    len -= c;
    hd->hd_data_off += c;
    
    if(hd->hd_data_off == hd->hd_data_len)
      htsbuf_data_free(hq, hd);
  }
  return r;
}

/**
 *
 */
void
htsbuf_vqprintf(htsbuf_queue_t *hq, const char *fmt, va_list ap)
{
  char buf[5000];
  htsbuf_append(hq, buf, vsnprintf(buf, sizeof(buf), fmt, ap));
}


/**
 *
 */
void
htsbuf_qprintf(htsbuf_queue_t *hq, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  htsbuf_vqprintf(hq, fmt, ap);
  va_end(ap);
}


