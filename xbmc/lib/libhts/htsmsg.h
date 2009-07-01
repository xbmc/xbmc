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
  /**
   * fields 
   */
  struct htsmsg_field_queue hm_fields;

  /**
   * Set if this message is a list, otherwise it is a map.
   */
  int hm_islist;

  /**
   * Data to be free'd when the message is destroyed
   */
  const void *hm_data;
} htsmsg_t;


#define HMF_MAP  1
#define HMF_S64 2
#define HMF_STR 3
#define HMF_BIN 4
#define HMF_LIST 5

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

#define htsmsg_get_map_by_field(f) \
 ((f)->hmf_type == HMF_MAP ? &(f)->hmf_msg : NULL)

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)

/**
 * Create a new map
 */
htsmsg_t *htsmsg_create_map(void);

/**
 * Create a new list
 */
htsmsg_t *htsmsg_create_list(void);

/**
 * Destroys a message (map or list)
 */
void htsmsg_destroy(htsmsg_t *msg);

/**
 * Add an integer field where source is unsigned 32 bit.
 */
void htsmsg_add_u32(htsmsg_t *msg, const char *name, uint32_t u32);

/**
 * Add an integer field where source is signed 32 bit.
 */
void htsmsg_add_s32(htsmsg_t *msg, const char *name,  int32_t s32);

/**
 * Add an integer field where source is signed 64 bit.
 */
void htsmsg_add_s64(htsmsg_t *msg, const char *name,  int64_t s64);

/**
 * Add a string field.
 */
void htsmsg_add_str(htsmsg_t *msg, const char *name, const char *str);

/**
 * Add an field where source is a list or map message.
 */
void htsmsg_add_msg(htsmsg_t *msg, const char *name, htsmsg_t *sub);

/**
 * Add an field where source is a list or map message.
 *
 * This function will not strdup() \p name but relies on the caller
 * to keep the string allocated for as long as the message is valid.
 */
void htsmsg_add_msg_extname(htsmsg_t *msg, const char *name, htsmsg_t *sub);

/**
 * Add an binary field. The data is copied to a malloced storage
 */
void htsmsg_add_bin(htsmsg_t *msg, const char *name, const void *bin,
		    size_t len);

/**
 * Add an binary field. The data is not copied, instead the caller
 * is responsible for keeping the data valid for as long as the message
 * is around.
 */
void htsmsg_add_binptr(htsmsg_t *msg, const char *name, const void *bin,
		       size_t len);

/**
 * Get an integer as an unsigned 32 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_u32(htsmsg_t *msg, const char *name, uint32_t *u32p);

/**
 * Get an integer as an signed 32 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_s32(htsmsg_t *msg, const char *name,  int32_t *s32p);

/**
 * Get an integer as an signed 64 bit integer.
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not an integer or
 *              out of range for the requested storage.
 */
int htsmsg_get_s64(htsmsg_t *msg, const char *name,  int64_t *s64p);

/**
 * Get pointer to a binary field. No copying of data is performed.
 *
 * @param binp Pointer to a void * that will be filled in with a pointer
 *             to the data
 * @param lenp Pointer to a size_t that will be filled with the size of
 *             the data
 *
 * @return HTSMSG_ERR_FIELD_NOT_FOUND - Field does not exist
 *         HTSMSG_ERR_CONVERSION_IMPOSSIBLE - Field is not a binary blob.
 */
int htsmsg_get_bin(htsmsg_t *msg, const char *name, const void **binp,
		   size_t *lenp);

/**
 * Get a field of type 'list'. No copying is done.
 *
 * @return NULL if the field can not be found or not of list type.
 *         Otherwise a htsmsg is returned.
 */
htsmsg_t *htsmsg_get_list(htsmsg_t *msg, const char *name);

/**
 * Get a field of type 'string'. No copying is done.
 *
 * @return NULL if the field can not be found or not of string type.
 *         Otherwise a pointer to the data is returned.
 */
const char *htsmsg_get_str(htsmsg_t *msg, const char *name);

/**
 * Get a field of type 'map'. No copying is done.
 *
 * @return NULL if the field can not be found or not of map type.
 *         Otherwise a htsmsg is returned.
 */
htsmsg_t *htsmsg_get_map(htsmsg_t *msg, const char *name);

/**
 * Traverse a hierarchy of htsmsg's to find a specific child.
 */
htsmsg_t *htsmsg_get_map_multi(htsmsg_t *msg, ...);

/**
 * Given the field \p f, return a string if it is of type string, otherwise
 * return NULL
 */
const char *htsmsg_field_get_string(htsmsg_field_t *f);

/**
 * Return the field \p name as an u32.
 *
 * @return An unsigned 32 bit integer or NULL if the field can not be found
 *         or if conversion is not possible.
 */
int htsmsg_get_u32_or_default(htsmsg_t *msg, const char *name, uint32_t def);

/**
 * Remove the given field called \p name from the message \p msg.
 */
int htsmsg_delete_field(htsmsg_t *msg, const char *name);

/**
 * Detach will remove the given field (and only if it is a list or map)
 * from the message and make it a 'standalone message'. This means
 * the the contents of the sub message will stay valid if the parent is
 * destroyed. The caller is responsible for freeing this new message.
 */
htsmsg_t *htsmsg_detach_submsg(htsmsg_field_t *f);

/**
 * Print a message to stdout. 
 */
void htsmsg_print(htsmsg_t *msg);

/**
 * Create a new field. Primarily intended for htsmsg internal functions.
 */
htsmsg_field_t *htsmsg_field_add(htsmsg_t *msg, const char *name,
				 int type, int flags);

/**
 * Clone a message.
 */
htsmsg_t *htsmsg_copy(htsmsg_t *src);

#define HTSMSG_FOREACH(f, msg) TAILQ_FOREACH(f, &(msg)->hm_fields, hmf_link)

#endif /* HTSMSG_H_ */
