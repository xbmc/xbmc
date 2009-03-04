/*
 *  Functions for manipulating HTS messages
 *  Copyright (C) 2007 Andreas Öman
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

#ifndef HTSMSG_H_
#define HTSMSG_H_

#include <inttypes.h>
#include "htsq.h"

#define HTSMSG_ERR_FIELD_NOT_FOUND       -1
#define HTSMSG_ERR_CONVERSION_IMPOSSIBLE -2

TAILQ_HEAD(htsmsg_field_queue, htsmsg_field);

typedef struct htsmsg {
  struct htsmsg_field_queue hm_fields;
  int hm_array;
  const void *hm_data;                /* Data to be free'd when message
					 is free'd */
} htsmsg_t;


#define HMF_MSG 1
#define HMF_S64 2
#define HMF_STR 3
#define HMF_BIN 4
#define HMF_VEC 5

typedef struct htsmsg_field {
  TAILQ_ENTRY(htsmsg_field) hmf_link;
  const char *hmf_name;
  uint8_t hmf_type;
  uint8_t hmf_flags;

#define HMF_ALLOCED 0x1
#define HMF_NAME_ALLOCED 0x2

  union {
    int64_t  s64;
    const char *str;
    struct {
      const char *data;
      size_t len;
    } bin;
    htsmsg_t msg;
  } u;
} htsmsg_field_t;

#define hmf_s64     u.s64
#define hmf_msg     u.msg
#define hmf_str     u.str
#define hmf_bin     u.bin.data
#define hmf_binsize u.bin.len

#define htsmsg_get_msg_by_field(f) \
 ((f)->hmf_type == HMF_MSG ? &(f)->hmf_msg : NULL)

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)

htsmsg_t *htsmsg_create(void);

htsmsg_t *htsmsg_create_array(void);

void htsmsg_destroy(htsmsg_t *msg);

void htsmsg_add_u32(htsmsg_t *msg, const char *name, uint32_t u32);
void htsmsg_add_s32(htsmsg_t *msg, const char *name,  int32_t s32);
void htsmsg_add_s64(htsmsg_t *msg, const char *name,  int64_t s64);
void htsmsg_add_str(htsmsg_t *msg, const char *name, const char *str);
void htsmsg_add_msg(htsmsg_t *msg, const char *name, htsmsg_t *sub);
void htsmsg_add_msg_extname(htsmsg_t *msg, const char *name, htsmsg_t *sub);

void htsmsg_add_bin(htsmsg_t *msg, const char *name, const void *bin,
		    size_t len);
void htsmsg_add_binptr(htsmsg_t *msg, const char *name, const void *bin,
		       size_t len);

int htsmsg_get_u32(htsmsg_t *msg, const char *name, uint32_t *u32p);
int htsmsg_get_s32(htsmsg_t *msg, const char *name,  int32_t *s32p);
int htsmsg_get_s64(htsmsg_t *msg, const char *name,  int64_t *s64p);
int htsmsg_get_bin(htsmsg_t *msg, const char *name, const void **binp,
		   size_t *lenp);
htsmsg_t *htsmsg_get_array(htsmsg_t *msg, const char *name);

const char *htsmsg_get_str(htsmsg_t *msg, const char *name);
htsmsg_t *htsmsg_get_msg(htsmsg_t *msg, const char *name);

htsmsg_t *htsmsg_get_msg_multi(htsmsg_t *msg, ...);

const char *htsmsg_field_get_string(htsmsg_field_t *f);

int htsmsg_get_u32_or_default(htsmsg_t *msg, const char *name, uint32_t def);

int htsmsg_delete_field(htsmsg_t *msg, const char *name);

htsmsg_t *htsmsg_detach_submsg(htsmsg_field_t *f);

void htsmsg_print(htsmsg_t *msg);

htsmsg_field_t *htsmsg_field_add(htsmsg_t *msg, const char *name,
				 int type, int flags);

htsmsg_t *htsmsg_copy(htsmsg_t *src);

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)

#endif /* HTSMSG_H_ */
