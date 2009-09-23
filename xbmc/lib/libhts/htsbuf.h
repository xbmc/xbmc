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

#ifndef HTSBUF_H__
#define HTSBUF_H__

#include <stdarg.h>
#include "htsq.h"
#include <inttypes.h>

TAILQ_HEAD(htsbuf_data_queue, htsbuf_data);

typedef struct htsbuf_data {
  TAILQ_ENTRY(htsbuf_data) hd_link;
  uint8_t *hd_data;
  unsigned int hd_data_size; /* Size of allocation hb_data */
  unsigned int hd_data_len;  /* Number of valid bytes from hd_data */
  unsigned int hd_data_off;  /* Offset in data, used for partial writes */
} htsbuf_data_t;

typedef struct htsbuf_queue {
  struct htsbuf_data_queue hq_q;
  unsigned int hq_size;
  unsigned int hq_maxsize;
} htsbuf_queue_t;  

void htsbuf_queue_init(htsbuf_queue_t *hq, unsigned int maxsize);

void htsbuf_queue_flush(htsbuf_queue_t *hq);

void htsbuf_vqprintf(htsbuf_queue_t *hq, const char *fmt, va_list ap);

void htsbuf_qprintf(htsbuf_queue_t *hq, const char *fmt, ...);

void htsbuf_append(htsbuf_queue_t *hq, const char *buf, size_t len);

void htsbuf_append_prealloc(htsbuf_queue_t *hq, const char *buf, size_t len);

void htsbuf_data_free(htsbuf_queue_t *hq, htsbuf_data_t *hd);

size_t htsbuf_read(htsbuf_queue_t *hq, char *buf, size_t len);

size_t htsbuf_peek(htsbuf_queue_t *hq, char *buf, size_t len);

size_t htsbuf_drop(htsbuf_queue_t *hq, size_t len);

size_t htsbuf_find(htsbuf_queue_t *hq, uint8_t v);

#endif /* HTSBUF_H__ */
