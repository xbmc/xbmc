#ifndef _TALLOC_H_
#define _TALLOC_H_
/* 
   Unix SMB/CIFS implementation.
   Samba temporary memory allocation functions

   Copyright (C) Andrew Tridgell 2004-2005
   
     ** NOTE! The following LGPL license applies to the talloc
     ** library. This does NOT imply that all of Samba is released
     ** under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* this is only needed for compatibility with the old talloc */
typedef void TALLOC_CTX;

/*
  this uses a little trick to allow __LINE__ to be stringified
*/
#define _STRING_LINE_(s)    #s
#define _STRING_LINE2_(s)   _STRING_LINE_(s)
#define __LINESTR__       _STRING_LINE2_(__LINE__)
#define __location__ __FILE__ ":" __LINESTR__

#ifndef TALLOC_DEPRECATED
#define TALLOC_DEPRECATED 0
#endif

/* useful macros for creating type checked pointers */
#define talloc(ctx, type) (type *)talloc_named_const(ctx, sizeof(type), #type)
#define talloc_size(ctx, size) talloc_named_const(ctx, size, __location__)

#define talloc_new(ctx) talloc_named_const(ctx, 0, "talloc_new: " __location__)

#define talloc_zero(ctx, type) (type *)_talloc_zero(ctx, sizeof(type), #type)
#define talloc_zero_size(ctx, size) _talloc_zero(ctx, size, __location__)

#define talloc_zero_array(ctx, type, count) (type *)_talloc_zero_array(ctx, sizeof(type), count, #type)
#define talloc_array(ctx, type, count) (type *)_talloc_array(ctx, sizeof(type), count, #type)
#define talloc_array_size(ctx, size, count) _talloc_array(ctx, size, count, __location__)

#define talloc_realloc(ctx, p, type, count) (type *)_talloc_realloc_array(ctx, p, sizeof(type), count, #type)
#define talloc_realloc_size(ctx, ptr, size) _talloc_realloc(ctx, ptr, size, __location__)

#define talloc_memdup(t, p, size) _talloc_memdup(t, p, size, __location__)

#define malloc_p(type) (type *)malloc(sizeof(type))
#define malloc_array_p(type, count) (type *)realloc_array(NULL, sizeof(type), count)
#define realloc_p(p, type, count) (type *)realloc_array(p, sizeof(type), count)

#if 0 
/* Not correct for Samba3. */
#define data_blob(ptr, size) data_blob_named(ptr, size, "DATA_BLOB: "__location__)
#define data_blob_talloc(ctx, ptr, size) data_blob_talloc_named(ctx, ptr, size, "DATA_BLOB: "__location__)
#define data_blob_dup_talloc(ctx, blob) data_blob_talloc_named(ctx, (blob)->data, (blob)->length, "DATA_BLOB: "__location__)
#endif

#define talloc_set_type(ptr, type) talloc_set_name_const(ptr, #type)
#define talloc_get_type(ptr, type) (type *)talloc_check_name(ptr, #type)

#define talloc_find_parent_bytype(ptr, type) (type *)talloc_find_parent_byname(ptr, #type)


#if TALLOC_DEPRECATED
#define talloc_zero_p(ctx, type) talloc_zero(ctx, type)
#define talloc_p(ctx, type) talloc(ctx, type)
#define talloc_array_p(ctx, type, count) talloc_array(ctx, type, count)
#define talloc_realloc_p(ctx, p, type, count) talloc_realloc(ctx, p, type, count)
#define talloc_destroy(ctx) talloc_free(ctx)
#endif

#ifndef PRINTF_ATTRIBUTE
#if (__GNUC__ >= 3)
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Note that some gcc 2.x versions don't handle this
 * properly **/
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif
#endif


/* The following definitions come from talloc.c  */
void *_talloc(const void *context, size_t size);
void talloc_set_destructor(const void *ptr, int (*destructor)(void *));
void talloc_increase_ref_count(const void *ptr);
void *talloc_reference(const void *context, const void *ptr);
int talloc_unlink(const void *context, void *ptr);
void talloc_set_name(const void *ptr, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);
void talloc_set_name_const(const void *ptr, const char *name);
void *talloc_named(const void *context, size_t size, 
		   const char *fmt, ...) PRINTF_ATTRIBUTE(3,4);
void *talloc_named_const(const void *context, size_t size, const char *name);
const char *talloc_get_name(const void *ptr);
void *talloc_check_name(const void *ptr, const char *name);
void talloc_report_depth(const void *ptr, FILE *f, int depth);
void *talloc_parent(const void *ptr);
void *talloc_init(const char *fmt, ...) PRINTF_ATTRIBUTE(1,2);
int talloc_free(void *ptr);
void *_talloc_realloc(const void *context, void *ptr, size_t size, const char *name);
void *talloc_steal(const void *new_ctx, const void *ptr);
long talloc_total_size(const void *ptr);
long talloc_total_blocks(const void *ptr);
void talloc_report_full(const void *ptr, FILE *f);
void talloc_report(const void *ptr, FILE *f);
void talloc_enable_null_tracking(void);
void talloc_enable_leak_report(void);
void talloc_enable_leak_report_full(void);
void *_talloc_zero(const void *ctx, size_t size, const char *name);
void *_talloc_memdup(const void *t, const void *p, size_t size, const char *name);
char *talloc_strdup(const void *t, const char *p);
char *talloc_strndup(const void *t, const char *p, size_t n);
char *talloc_append_string(const void *t, char *orig, const char *append);
char *talloc_vasprintf(const void *t, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);
char *talloc_asprintf(const void *t, const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);
char *talloc_asprintf_append(char *s,
			     const char *fmt, ...) PRINTF_ATTRIBUTE(2,3);
void *_talloc_array(const void *ctx, size_t el_size, unsigned count, const char *name);
void *_talloc_zero_array(const void *ctx, size_t el_size, unsigned count, const char *name);
void *_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name);
void *talloc_realloc_fn(const void *context, void *ptr, size_t size);
void *talloc_autofree_context(void);
size_t talloc_get_size(const void *ctx);
void *talloc_find_parent_byname(const void *ctx, const char *name);
void talloc_show_parents(const void *context, FILE *file);
int talloc_is_parent(const void *context, const char *ptr);

#endif

