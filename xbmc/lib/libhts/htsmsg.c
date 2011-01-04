/*
 *  Functions for manipulating HTS messages
 *  Copyright (C) 2007 Andreas Öman
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _MSC_VER
#include "msvc.h"
#endif

#include "htsmsg.h"

static void htsmsg_clear(htsmsg_t *msg);

/*
 *
 */
static void
htsmsg_field_destroy(htsmsg_t *msg, htsmsg_field_t *f)
{
  TAILQ_REMOVE(&msg->hm_fields, f, hmf_link);

  switch(f->hmf_type) {
  case HMF_MAP:
  case HMF_LIST:
    htsmsg_clear(&f->hmf_msg);
    break;

  case HMF_STR:
    if(f->hmf_flags & HMF_ALLOCED)
      free((void *)f->hmf_str);
    break;

  case HMF_BIN:
    if(f->hmf_flags & HMF_ALLOCED)
      free((void *)f->hmf_bin);
    break;
  default:
    break;
  }
  if(f->hmf_flags & HMF_NAME_ALLOCED)
    free((void *)f->hmf_name);
  free(f);
}

/*
 *
 */
static void
htsmsg_clear(htsmsg_t *msg)
{
  htsmsg_field_t *f;

  while((f = TAILQ_FIRST(&msg->hm_fields)) != NULL)
    htsmsg_field_destroy(msg, f);
}



/*
 *
 */
htsmsg_field_t *
htsmsg_field_add(htsmsg_t *msg, const char *name, int type, int flags)
{
  htsmsg_field_t *f = malloc(sizeof(htsmsg_field_t));
  
  TAILQ_INSERT_TAIL(&msg->hm_fields, f, hmf_link);

  if(msg->hm_islist) {
    assert(name == NULL);
  } else {
    assert(name != NULL);
  }

  if(flags & HMF_NAME_ALLOCED)
    f->hmf_name = name ? strdup(name) : NULL;
  else
    f->hmf_name = name;

  f->hmf_type = type;
  f->hmf_flags = flags;
  return f;
}


/*
 *
 */
static htsmsg_field_t *
htsmsg_field_find(htsmsg_t *msg, const char *name)
{
  htsmsg_field_t *f;

  TAILQ_FOREACH(f, &msg->hm_fields, hmf_link) {
    if(f->hmf_name != NULL && !strcmp(f->hmf_name, name))
      return f;
  }
  return NULL;
}



/**
 *
 */
int
htsmsg_delete_field(htsmsg_t *msg, const char *name)
{
  htsmsg_field_t *f;

  if((f = htsmsg_field_find(msg, name)) == NULL)
    return HTSMSG_ERR_FIELD_NOT_FOUND;
  htsmsg_field_destroy(msg, f);
  return 0;
}


/*
 *
 */
htsmsg_t *
htsmsg_create_map(void)
{
  htsmsg_t *msg;

  msg = malloc(sizeof(htsmsg_t));
  TAILQ_INIT(&msg->hm_fields);
  msg->hm_data = NULL;
  msg->hm_islist = 0;
  return msg;
}

/*
 *
 */
htsmsg_t *
htsmsg_create_list(void)
{
  htsmsg_t *msg;

  msg = malloc(sizeof(htsmsg_t));
  TAILQ_INIT(&msg->hm_fields);
  msg->hm_data = NULL;
  msg->hm_islist = 1;
  return msg;
}


/*
 *
 */
void
htsmsg_destroy(htsmsg_t *msg)
{
  if(msg == NULL)
    return;

  htsmsg_clear(msg);
  free((void *)msg->hm_data);
  free(msg);
}

/*
 *
 */
void
htsmsg_add_u32(htsmsg_t *msg, const char *name, uint32_t u32)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_S64, HMF_NAME_ALLOCED);
  f->hmf_s64 = u32;
}

/*
 *
 */
void
htsmsg_add_s64(htsmsg_t *msg, const char *name, int64_t s64)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_S64, HMF_NAME_ALLOCED);
  f->hmf_s64 = s64;
}

/*
 *
 */
void
htsmsg_add_s32(htsmsg_t *msg, const char *name, int32_t s32)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_S64, HMF_NAME_ALLOCED);
  f->hmf_s64 = s32;
}



/*
 *
 */
void
htsmsg_add_str(htsmsg_t *msg, const char *name, const char *str)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_STR, 
				        HMF_ALLOCED | HMF_NAME_ALLOCED);
  f->hmf_str = strdup(str);
}

/*
 *
 */
void
htsmsg_add_bin(htsmsg_t *msg, const char *name, const void *bin, size_t len)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_BIN, 
				       HMF_ALLOCED | HMF_NAME_ALLOCED);
  void *v;
  f->hmf_bin = v = malloc(len);
  f->hmf_binsize = len;
  memcpy(v, bin, len);
}

/*
 *
 */
void
htsmsg_add_binptr(htsmsg_t *msg, const char *name, const void *bin, size_t len)
{
  htsmsg_field_t *f = htsmsg_field_add(msg, name, HMF_BIN, HMF_NAME_ALLOCED);
  f->hmf_bin = bin;
  f->hmf_binsize = len;
}


/*
 *
 */
void
htsmsg_add_msg(htsmsg_t *msg, const char *name, htsmsg_t *sub)
{
  htsmsg_field_t *f;

  f = htsmsg_field_add(msg, name, sub->hm_islist ? HMF_LIST : HMF_MAP,
		       HMF_NAME_ALLOCED);

  assert(sub->hm_data == NULL);
  TAILQ_MOVE(&f->hmf_msg.hm_fields, &sub->hm_fields, hmf_link);
  free(sub);
}



/*
 *
 */
void
htsmsg_add_msg_extname(htsmsg_t *msg, const char *name, htsmsg_t *sub)
{
  htsmsg_field_t *f;

  f = htsmsg_field_add(msg, name, sub->hm_islist ? HMF_LIST : HMF_MAP, 0);

  assert(sub->hm_data == NULL);
  TAILQ_MOVE(&f->hmf_msg.hm_fields, &sub->hm_fields, hmf_link);
  free(sub);
}



/**
 *
 */
int
htsmsg_get_s64(htsmsg_t *msg, const char *name, int64_t *s64p)
{
  htsmsg_field_t *f;

  if((f = htsmsg_field_find(msg, name)) == NULL)
    return HTSMSG_ERR_FIELD_NOT_FOUND;

  switch(f->hmf_type) {
  default:
    return HTSMSG_ERR_CONVERSION_IMPOSSIBLE;
  case HMF_STR:
    *s64p = strtoll(f->hmf_str, NULL, 0);
    break;
  case HMF_S64:
    *s64p = f->hmf_s64;
    break;
  }
  return 0;
}


/*
 *
 */
int
htsmsg_get_u32(htsmsg_t *msg, const char *name, uint32_t *u32p)
{
  int r;
  int64_t s64;

  if((r = htsmsg_get_s64(msg, name, &s64)) != 0)
    return r;

  if(s64 < 0 || s64 > 0xffffffffLL)
    return HTSMSG_ERR_CONVERSION_IMPOSSIBLE;
  
  *u32p = (uint32_t)s64;
  return 0;
}

/**
 *
 */
int
htsmsg_get_u32_or_default(htsmsg_t *msg, const char *name, uint32_t def)
{
  uint32_t u32;
    return htsmsg_get_u32(msg, name, &u32) ? def : u32;
}



/*
 *
 */
int
htsmsg_get_s32(htsmsg_t *msg, const char *name, int32_t *s32p)
{
  int r;
  int64_t s64;

  if((r = htsmsg_get_s64(msg, name, &s64)) != 0)
    return r;

  if(s64 < -0x80000000LL || s64 > 0x7fffffffLL)
    return HTSMSG_ERR_CONVERSION_IMPOSSIBLE;
  
  *s32p = (int32_t)s64;
  return 0;
}


/*
 *
 */
int
htsmsg_get_bin(htsmsg_t *msg, const char *name, const void **binp,
	       size_t *lenp)
{
  htsmsg_field_t *f;
  
  if((f = htsmsg_field_find(msg, name)) == NULL)
    return HTSMSG_ERR_FIELD_NOT_FOUND;
  
  if(f->hmf_type != HMF_BIN)
    return HTSMSG_ERR_CONVERSION_IMPOSSIBLE;

  *binp = f->hmf_bin;
  *lenp = f->hmf_binsize;
  return 0;
}

/**
 *
 */
const char *
htsmsg_field_get_string(htsmsg_field_t *f)
{
  char buf[40];
  
  switch(f->hmf_type) {
  default:
    return NULL;
  case HMF_STR:
    break;
  case HMF_S64:
    snprintf(buf, sizeof(buf), "%"PRId64, f->hmf_s64);
    f->hmf_str = strdup(buf);
    f->hmf_type = HMF_STR;
    break;
  }
  return f->hmf_str;
}

/*
 *
 */
const char *
htsmsg_get_str(htsmsg_t *msg, const char *name)
{
  htsmsg_field_t *f;

  if((f = htsmsg_field_find(msg, name)) == NULL)
    return NULL;
  return htsmsg_field_get_string(f);

}

/*
 *
 */
htsmsg_t *
htsmsg_get_map(htsmsg_t *msg, const char *name)
{
  htsmsg_field_t *f;

  if((f = htsmsg_field_find(msg, name)) == NULL || f->hmf_type != HMF_MAP)
    return NULL;

  return &f->hmf_msg;
}

/**
 *
 */
htsmsg_t *
htsmsg_get_map_multi(htsmsg_t *msg, ...)
{
  va_list ap;
  const char *n;
  va_start(ap, msg);

  while(msg != NULL && (n = va_arg(ap, char *)) != NULL)
    msg = htsmsg_get_map(msg, n);
  return msg;
}

/*
 *
 */
htsmsg_t *
htsmsg_get_list(htsmsg_t *msg, const char *name)
{
  htsmsg_field_t *f;

  if((f = htsmsg_field_find(msg, name)) == NULL || f->hmf_type != HMF_LIST)
    return NULL;

  return &f->hmf_msg;
}

/**
 *
 */
htsmsg_t *
htsmsg_detach_submsg(htsmsg_field_t *f)
{
  htsmsg_t *r = htsmsg_create_map();

  TAILQ_MOVE(&r->hm_fields, &f->hmf_msg.hm_fields, hmf_link);
  TAILQ_INIT(&f->hmf_msg.hm_fields);
  r->hm_islist = f->hmf_type == HMF_LIST;
  return r;
}


/*
 *
 */
static void
htsmsg_print0(htsmsg_t *msg, int indent)
{
  htsmsg_field_t *f;
  int i;

  TAILQ_FOREACH(f, &msg->hm_fields, hmf_link) {

    for(i = 0; i < indent; i++) printf("\t");
    
    printf("%s (", f->hmf_name ? f->hmf_name : "");
    
    switch(f->hmf_type) {

    case HMF_MAP:
      printf("MAP) = {\n");
      htsmsg_print0(&f->hmf_msg, indent + 1);
      for(i = 0; i < indent; i++) printf("\t"); printf("}\n");
      break;

    case HMF_LIST:
      printf("LIST) = {\n");
      htsmsg_print0(&f->hmf_msg, indent + 1);
      for(i = 0; i < indent; i++) printf("\t"); printf("}\n");
      break;
      
    case HMF_STR:
      printf("STR) = \"%s\"\n", f->hmf_str);
      break;

    case HMF_BIN:
      printf("BIN) = [");
      for(i = 0; i < (int)f->hmf_binsize - 1; i++)
	printf("%02x.", ((uint8_t *)f->hmf_bin)[i]);
      printf("%02x]\n", ((uint8_t *)f->hmf_bin)[i]);
      break;

    case HMF_S64:
      printf("S64) = %" PRId64 "\n", f->hmf_s64);
      break;
    }
  }
} 

/*
 *
 */
void
htsmsg_print(htsmsg_t *msg)
{
  htsmsg_print0(msg, 0);
} 


/**
 *
 */
static void
htsmsg_copy_i(htsmsg_t *src, htsmsg_t *dst)
{
  htsmsg_field_t *f;
  htsmsg_t *sub;

  TAILQ_FOREACH(f, &src->hm_fields, hmf_link) {

    switch(f->hmf_type) {

    case HMF_MAP:
    case HMF_LIST:
      sub = f->hmf_type == HMF_LIST ? 
	htsmsg_create_list() : htsmsg_create_map();
      htsmsg_copy_i(&f->hmf_msg, sub);
      htsmsg_add_msg(dst, f->hmf_name, sub);
      break;
      
    case HMF_STR:
      htsmsg_add_str(dst, f->hmf_name, f->hmf_str);
      break;

    case HMF_S64:
      htsmsg_add_s64(dst, f->hmf_name, f->hmf_s64);
      break;

    case HMF_BIN:
      htsmsg_add_bin(dst, f->hmf_name, f->hmf_bin, f->hmf_binsize);
      break;
    }
  }
}

htsmsg_t *
htsmsg_copy(htsmsg_t *src)
{
  htsmsg_t *dst = src->hm_islist ? htsmsg_create_list() : htsmsg_create_map();
  htsmsg_copy_i(src, dst);
  return dst;
}
