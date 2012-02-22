/*
 *  Functions converting HTSMSGs to/from a simple binary format
 *  Copyright (C) 2007 Andreas �man
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
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "htsmsg_binary.h"

/*
 *
 */
static int
htsmsg_binary_des0(htsmsg_t *msg, const uint8_t *buf, size_t len)
{
  unsigned type, namelen, datalen;
  htsmsg_field_t *f;
  htsmsg_t *sub;
  char *n;
  uint64_t u64;
  int i;

  while(len > 5) {

    type    =  buf[0];
    namelen =  buf[1];
    datalen = (buf[2] << 24) |
              (buf[3] << 16) |
              (buf[4] << 8 ) |
              (buf[5]      );
    
    buf += 6;
    len -= 6;
    
    if(len < namelen + datalen)
      return -1;

    f = malloc(sizeof(htsmsg_field_t));
    f->hmf_type  = type;

    if(namelen > 0) {
      n = malloc(namelen + 1);
      memcpy(n, buf, namelen);
      n[namelen] = 0;

      buf += namelen;
      len -= namelen;
      f->hmf_flags = HMF_NAME_ALLOCED;

    } else {
      n = NULL;
      f->hmf_flags = 0;
    }

    f->hmf_name  = n;

    switch(type) {
    case HMF_STR:
      f->hmf_str = n = malloc(datalen + 1);
      memcpy(n, buf, datalen);
      n[datalen] = 0;
      f->hmf_flags |= HMF_ALLOCED;
      break;

    case HMF_BIN:
      f->hmf_bin = (const void *)buf;
      f->hmf_binsize = datalen;
      break;

    case HMF_S64:
      u64 = 0;
      for(i = datalen - 1; i >= 0; i--)
          u64 = (u64 << 8) | buf[i];
      f->hmf_s64 = u64;
      break;

    case HMF_MAP:
    case HMF_LIST:
      sub = &f->hmf_msg;
      TAILQ_INIT(&sub->hm_fields);
      sub->hm_data = NULL;
      if(htsmsg_binary_des0(sub, buf, datalen) < 0)
        return -1;
      break;

    default:
      free(n);
      free(f);
      return -1;
    }

    TAILQ_INSERT_TAIL(&msg->hm_fields, f, hmf_link);
    buf += datalen;
    len -= datalen;
  }
  return 0;
}



/*
 *
 */
htsmsg_t *
htsmsg_binary_deserialize(const void *data, size_t len, const void *buf)
{
  htsmsg_t *msg = htsmsg_create_map();
  msg->hm_data = buf;

  if(htsmsg_binary_des0(msg, data, len) < 0) {
    htsmsg_destroy(msg);
    return NULL;
  }
  return msg;
}



/*
 *
 */
static size_t
htsmsg_binary_count(htsmsg_t *msg)
{
  htsmsg_field_t *f;
  size_t len = 0;
  uint64_t u64;

  TAILQ_FOREACH(f, &msg->hm_fields, hmf_link) {

    len += 6;
    len += f->hmf_name ? strlen(f->hmf_name) : 0;

    switch(f->hmf_type) {
    case HMF_MAP:
    case HMF_LIST:
      len += htsmsg_binary_count(&f->hmf_msg);
      break;

    case HMF_STR:
      len += strlen(f->hmf_str);
      break;

    case HMF_BIN:
      len += f->hmf_binsize;
      break;

    case HMF_S64:
      u64 = f->hmf_s64;
      while(u64 != 0) {
        len++;
        u64 = u64 >> 8;
      }
      break;
    }
  }
  return len;
}


/*
 *
 */
static void
htsmsg_binary_write(htsmsg_t *msg, uint8_t *ptr)
{
  htsmsg_field_t *f;
  uint64_t u64;
  int l, i, namelen;

  TAILQ_FOREACH(f, &msg->hm_fields, hmf_link) {
    namelen = f->hmf_name ? strlen(f->hmf_name) : 0;
    *ptr++ = f->hmf_type;
    *ptr++ = namelen;

    switch(f->hmf_type) {
    case HMF_MAP:
    case HMF_LIST:
      l = htsmsg_binary_count(&f->hmf_msg);
      break;

    case HMF_STR:
      l = strlen(f->hmf_str);
      break;

    case HMF_BIN:
      l = f->hmf_binsize;
      break;

    case HMF_S64:
      u64 = f->hmf_s64;
      l = 0;
      while(u64 != 0) {
        l++;
        u64 = u64 >> 8;
      }
      break;
    default:
      abort();
    }


    *ptr++ = l >> 24;
    *ptr++ = l >> 16;
    *ptr++ = l >> 8;
    *ptr++ = l;

    if(namelen > 0) {
      memcpy(ptr, f->hmf_name, namelen);
      ptr += namelen;
    }

    switch(f->hmf_type) {
    case HMF_MAP:
    case HMF_LIST:
      htsmsg_binary_write(&f->hmf_msg, ptr);
      break;

    case HMF_STR:
      memcpy(ptr, f->hmf_str, l);
      break;

    case HMF_BIN:
      memcpy(ptr, f->hmf_bin, l);
      break;

    case HMF_S64:
      u64 = f->hmf_s64;
      for(i = 0; i < l; i++) {
        ptr[i] = u64;
        u64 = u64 >> 8;
      }
      break;
    }
    ptr += l;
  }
}


/*
 *
 */
int
htsmsg_binary_serialize(htsmsg_t *msg, void **datap, size_t *lenp, int maxlen)
{
  size_t len;
  uint8_t *data;

  len = htsmsg_binary_count(msg);
  if(len + 4 > maxlen)
    return -1;

  data = malloc(len + 4);

  data[0] = len >> 24;
  data[1] = len >> 16;
  data[2] = len >> 8;
  data[3] = len;

  htsmsg_binary_write(msg, data + 4);
  *datap = data;
  *lenp  = len + 4;
  return 0;
}
