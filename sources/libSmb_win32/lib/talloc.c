/*
   Samba Unix SMB/CIFS implementation.

   Samba trivial allocation library - new interface

   NOTE: Please read talloc_guide.txt for full documentation

   Copyright (C) Andrew Tridgell 2004

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

/*
  inspired by http://swapped.cc/halloc/
*/

#ifdef _SAMBA_BUILD_
#include "includes.h"
#if ((SAMBA_VERSION_MAJOR==3)&&(SAMBA_VERSION_MINOR<9))
/* This is to circumvent SAMBA3's paranoid malloc checker. Here in this file
 * we trust ourselves... */
#ifdef malloc
#undef malloc
#endif
#ifdef realloc
#undef realloc
#endif
#endif
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _XBOX
#include <stdint.h>
#endif
#include "talloc.h"
#ifdef _XBOX
#undef HAVE_VA_COPY
#define vsnprintf _vsnprintf
#else
/* assume a modern system */
#define HAVE_VA_COPY
#endif
#endif

/* use this to force every realloc to change the pointer, to stress test
   code that might not cope */
#define ALWAYS_REALLOC 0


#define MAX_TALLOC_SIZE 0x10000000
#define TALLOC_MAGIC 0xe814ec70
#define TALLOC_FLAG_FREE 0x01
#define TALLOC_FLAG_LOOP 0x02
#define TALLOC_MAGIC_REFERENCE ((const char *)1)

/* by default we abort when given a bad pointer (such as when talloc_free() is called 
   on a pointer that came from malloc() */
#ifndef TALLOC_ABORT
#define TALLOC_ABORT(reason) abort()
#endif

#ifndef discard_const_p
#if defined(__intptr_t_defined) || defined(HAVE_INTPTR_T)
# define discard_const_p(type, ptr) ((type *)((intptr_t)(ptr)))
#else
# define discard_const_p(type, ptr) ((type *)(ptr))
#endif
#endif

/* this null_context is only used if talloc_enable_leak_report() or
   talloc_enable_leak_report_full() is called, otherwise it remains
   NULL
*/
static const void *null_context;
static void *cleanup_context;


struct talloc_reference_handle {
	struct talloc_reference_handle *next, *prev;
	void *ptr;
};

typedef int (*talloc_destructor_t)(void *);

struct talloc_chunk {
	struct talloc_chunk *next, *prev;
	struct talloc_chunk *parent, *child;
	struct talloc_reference_handle *refs;
	talloc_destructor_t destructor;
	const char *name;
	size_t size;
	unsigned flags;
};

/* 16 byte alignment seems to keep everyone happy */
#define TC_HDR_SIZE ((sizeof(struct talloc_chunk)+15)&~15)
#define TC_PTR_FROM_CHUNK(tc) ((void *)(TC_HDR_SIZE + (char*)tc))

/* panic if we get a bad magic value */
static struct talloc_chunk *talloc_chunk_from_ptr(const void *ptr)
{
	const char *pp = ptr;
	struct talloc_chunk *tc = discard_const_p(struct talloc_chunk, pp - TC_HDR_SIZE);
	if ((tc->flags & ~0xF) != TALLOC_MAGIC) { 
		TALLOC_ABORT("Bad talloc magic value - unknown value"); 
	}
	if (tc->flags & TALLOC_FLAG_FREE) {
		TALLOC_ABORT("Bad talloc magic value - double free"); 
	}
	return tc;
}

/* hook into the front of the list */
#define _TLIST_ADD(list, p) \
do { \
        if (!(list)) { \
		(list) = (p); \
		(p)->next = (p)->prev = NULL; \
	} else { \
		(list)->prev = (p); \
		(p)->next = (list); \
		(p)->prev = NULL; \
		(list) = (p); \
	}\
} while (0)

/* remove an element from a list - element doesn't have to be in list. */
#define _TLIST_REMOVE(list, p) \
do { \
	if ((p) == (list)) { \
		(list) = (p)->next; \
		if (list) (list)->prev = NULL; \
	} else { \
		if ((p)->prev) (p)->prev->next = (p)->next; \
		if ((p)->next) (p)->next->prev = (p)->prev; \
	} \
	if ((p) && ((p) != (list))) (p)->next = (p)->prev = NULL; \
} while (0)


/*
  return the parent chunk of a pointer
*/
static struct talloc_chunk *talloc_parent_chunk(const void *ptr)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	while (tc->prev) tc=tc->prev;
	return tc->parent;
}

void *talloc_parent(const void *ptr)
{
	struct talloc_chunk *tc = talloc_parent_chunk(ptr);
	return tc? TC_PTR_FROM_CHUNK(tc) : NULL;
}

/* 
   Allocate a bit of memory as a child of an existing pointer
*/
void *_talloc(const void *context, size_t size)
{
	struct talloc_chunk *tc;

	if (context == NULL) {
		context = null_context;
	}

	if (size >= MAX_TALLOC_SIZE) {
		return NULL;
	}

	tc = malloc(TC_HDR_SIZE+size);
	if (tc == NULL) return NULL;

	tc->size = size;
	tc->flags = TALLOC_MAGIC;
	tc->destructor = NULL;
	tc->child = NULL;
	tc->name = NULL;
	tc->refs = NULL;

	if (context) {
		struct talloc_chunk *parent = talloc_chunk_from_ptr(context);

		tc->parent = parent;

		if (parent->child) {
			parent->child->parent = NULL;
		}

		_TLIST_ADD(parent->child, tc);
	} else {
		tc->next = tc->prev = tc->parent = NULL;
	}

	return TC_PTR_FROM_CHUNK(tc);
}


/*
  setup a destructor to be called on free of a pointer
  the destructor should return 0 on success, or -1 on failure.
  if the destructor fails then the free is failed, and the memory can
  be continued to be used
*/
void talloc_set_destructor(const void *ptr, int (*destructor)(void *))
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	tc->destructor = destructor;
}

/*
  increase the reference count on a piece of memory. 
*/
void talloc_increase_ref_count(const void *ptr)
{
	talloc_reference(null_context, ptr);
}

/*
  helper for talloc_reference()
*/
static int talloc_reference_destructor(void *ptr)
{
	struct talloc_reference_handle *handle = ptr;
	struct talloc_chunk *tc1 = talloc_chunk_from_ptr(ptr);
	struct talloc_chunk *tc2 = talloc_chunk_from_ptr(handle->ptr);
	if (tc1->destructor != (talloc_destructor_t)-1) {
		tc1->destructor = NULL;
	}
	_TLIST_REMOVE(tc2->refs, handle);
	talloc_free(handle);
	return 0;
}

/*
  make a secondary reference to a pointer, hanging off the given context.
  the pointer remains valid until both the original caller and this given
  context are freed.
  
  the major use for this is when two different structures need to reference the 
  same underlying data, and you want to be able to free the two instances separately,
  and in either order
*/
void *talloc_reference(const void *context, const void *ptr)
{
	struct talloc_chunk *tc;
	struct talloc_reference_handle *handle;
	if (ptr == NULL) return NULL;

	tc = talloc_chunk_from_ptr(ptr);
	handle = talloc_named_const(context, sizeof(*handle), TALLOC_MAGIC_REFERENCE);

	if (handle == NULL) return NULL;

	/* note that we hang the destructor off the handle, not the
	   main context as that allows the caller to still setup their
	   own destructor on the context if they want to */
	talloc_set_destructor(handle, talloc_reference_destructor);
	handle->ptr = discard_const_p(void, ptr);
	_TLIST_ADD(tc->refs, handle);
	return handle->ptr;
}

/*
  remove a secondary reference to a pointer. This undo's what
  talloc_reference() has done. The context and pointer arguments
  must match those given to a talloc_reference()
*/
static int talloc_unreference(const void *context, const void *ptr)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	struct talloc_reference_handle *h;

	if (context == NULL) {
		context = null_context;
	}

	for (h=tc->refs;h;h=h->next) {
		struct talloc_chunk *p = talloc_parent_chunk(h);
		if (p == NULL) {
			if (context == NULL) break;
		} else if (TC_PTR_FROM_CHUNK(p) == context) {
			break;
		}
	}
	if (h == NULL) {
		return -1;
	}

	talloc_set_destructor(h, NULL);
	_TLIST_REMOVE(tc->refs, h);
	talloc_free(h);
	return 0;
}

/*
  remove a specific parent context from a pointer. This is a more
  controlled varient of talloc_free()
*/
int talloc_unlink(const void *context, void *ptr)
{
	struct talloc_chunk *tc_p, *new_p;
	void *new_parent;

	if (ptr == NULL) {
		return -1;
	}

	if (context == NULL) {
		context = null_context;
	}

	if (talloc_unreference(context, ptr) == 0) {
		return 0;
	}

	if (context == NULL) {
		if (talloc_parent_chunk(ptr) != NULL) {
			return -1;
		}
	} else {
		if (talloc_chunk_from_ptr(context) != talloc_parent_chunk(ptr)) {
			return -1;
		}
	}
	
	tc_p = talloc_chunk_from_ptr(ptr);

	if (tc_p->refs == NULL) {
		return talloc_free(ptr);
	}

	new_p = talloc_parent_chunk(tc_p->refs);
	if (new_p) {
		new_parent = TC_PTR_FROM_CHUNK(new_p);
	} else {
		new_parent = NULL;
	}

	if (talloc_unreference(new_parent, ptr) != 0) {
		return -1;
	}

	talloc_steal(new_parent, ptr);

	return 0;
}

/*
  add a name to an existing pointer - va_list version
*/
static void talloc_set_name_v(const void *ptr, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);

static void talloc_set_name_v(const void *ptr, const char *fmt, va_list ap)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	tc->name = talloc_vasprintf(ptr, fmt, ap);
	if (tc->name) {
		talloc_set_name_const(tc->name, ".name");
	}
}

/*
  add a name to an existing pointer
*/
void talloc_set_name(const void *ptr, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);
}

/*
   more efficient way to add a name to a pointer - the name must point to a 
   true string constant
*/
void talloc_set_name_const(const void *ptr, const char *name)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	tc->name = name;
}

/*
  create a named talloc pointer. Any talloc pointer can be named, and
  talloc_named() operates just like talloc() except that it allows you
  to name the pointer.
*/
void *talloc_named(const void *context, size_t size, const char *fmt, ...)
{
	va_list ap;
	void *ptr;

	ptr = _talloc(context, size);
	if (ptr == NULL) return NULL;

	va_start(ap, fmt);
	talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);

	return ptr;
}

/*
  create a named talloc pointer. Any talloc pointer can be named, and
  talloc_named() operates just like talloc() except that it allows you
  to name the pointer.
*/
void *talloc_named_const(const void *context, size_t size, const char *name)
{
	void *ptr;

	ptr = _talloc(context, size);
	if (ptr == NULL) {
		return NULL;
	}

	talloc_set_name_const(ptr, name);

	return ptr;
}

/*
  return the name of a talloc ptr, or "UNNAMED"
*/
const char *talloc_get_name(const void *ptr)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	if (tc->name == TALLOC_MAGIC_REFERENCE) {
		return ".reference";
	}
	if (tc->name) {
		return tc->name;
	}
	return "UNNAMED";
}


/*
  check if a pointer has the given name. If it does, return the pointer,
  otherwise return NULL
*/
void *talloc_check_name(const void *ptr, const char *name)
{
	const char *pname;
	if (ptr == NULL) return NULL;
	pname = talloc_get_name(ptr);
	if (pname == name || strcmp(pname, name) == 0) {
		return discard_const_p(void, ptr);
	}
	return NULL;
}


/*
  this is for compatibility with older versions of talloc
*/
void *talloc_init(const char *fmt, ...)
{
	va_list ap;
	void *ptr;

	talloc_enable_null_tracking();

	ptr = _talloc(NULL, 0);
	if (ptr == NULL) return NULL;

	va_start(ap, fmt);
	talloc_set_name_v(ptr, fmt, ap);
	va_end(ap);

	return ptr;
}

/*
  this is a replacement for the Samba3 talloc_destroy_pool functionality. It
  should probably not be used in new code. It's in here to keep the talloc
  code consistent across Samba 3 and 4.
*/
void talloc_free_children(void *ptr)
{
	struct talloc_chunk *tc;

	if (ptr == NULL) {
		return;
	}

	tc = talloc_chunk_from_ptr(ptr);

	while (tc->child) {
		/* we need to work out who will own an abandoned child
		   if it cannot be freed. In priority order, the first
		   choice is owner of any remaining reference to this
		   pointer, the second choice is our parent, and the
		   final choice is the null context. */
		void *child = TC_PTR_FROM_CHUNK(tc->child);
		const void *new_parent = null_context;
		if (tc->child->refs) {
			struct talloc_chunk *p = talloc_parent_chunk(tc->child->refs);
			if (p) new_parent = TC_PTR_FROM_CHUNK(p);
		}
		if (talloc_free(child) == -1) {
			if (new_parent == null_context) {
				struct talloc_chunk *p = talloc_parent_chunk(ptr);
				if (p) new_parent = TC_PTR_FROM_CHUNK(p);
			}
			talloc_steal(new_parent, child);
		}
	}
}

/* 
   free a talloc pointer. This also frees all child pointers of this 
   pointer recursively

   return 0 if the memory is actually freed, otherwise -1. The memory
   will not be freed if the ref_count is > 1 or the destructor (if
   any) returns non-zero
*/
int talloc_free(void *ptr)
{
	struct talloc_chunk *tc;

	if (ptr == NULL) {
		return -1;
	}

	tc = talloc_chunk_from_ptr(ptr);

	if (tc->refs) {
		int is_child;
		struct talloc_reference_handle *handle = tc->refs;
		is_child = talloc_is_parent(handle, handle->ptr);
		talloc_reference_destructor(tc->refs);
		if (is_child) {
			return talloc_free(ptr);
		}
		return -1;
	}

	if (tc->flags & TALLOC_FLAG_LOOP) {
		/* we have a free loop - stop looping */
		return 0;
	}

	if (tc->destructor) {
		talloc_destructor_t d = tc->destructor;
		if (d == (talloc_destructor_t)-1) {
			return -1;
		}
		tc->destructor = (talloc_destructor_t)-1;
		if (d(ptr) == -1) {
			tc->destructor = d;
			return -1;
		}
		tc->destructor = NULL;
	}

	tc->flags |= TALLOC_FLAG_LOOP;

	talloc_free_children(ptr);

	if (tc->parent) {
		_TLIST_REMOVE(tc->parent->child, tc);
		if (tc->parent->child) {
			tc->parent->child->parent = tc->parent;
		}
	} else {
		if (tc->prev) tc->prev->next = tc->next;
		if (tc->next) tc->next->prev = tc->prev;
	}

	tc->flags |= TALLOC_FLAG_FREE;

	free(tc);
	return 0;
}



/*
  A talloc version of realloc. The context argument is only used if
  ptr is NULL
*/
void *_talloc_realloc(const void *context, void *ptr, size_t size, const char *name)
{
	struct talloc_chunk *tc;
	void *new_ptr;

	/* size zero is equivalent to free() */
	if (size == 0) {
		talloc_free(ptr);
		return NULL;
	}

	if (size >= MAX_TALLOC_SIZE) {
		return NULL;
	}

	/* realloc(NULL) is equavalent to malloc() */
	if (ptr == NULL) {
		return talloc_named_const(context, size, name);
	}

	tc = talloc_chunk_from_ptr(ptr);

	/* don't allow realloc on referenced pointers */
	if (tc->refs) {
		return NULL;
	}

	/* by resetting magic we catch users of the old memory */
	tc->flags |= TALLOC_FLAG_FREE;

#if ALWAYS_REALLOC
	new_ptr = malloc(size + TC_HDR_SIZE);
	if (new_ptr) {
		memcpy(new_ptr, tc, tc->size + TC_HDR_SIZE);
		free(tc);
	}
#else
	new_ptr = realloc(tc, size + TC_HDR_SIZE);
#endif
	if (!new_ptr) {	
		tc->flags &= ~TALLOC_FLAG_FREE; 
		return NULL; 
	}

	tc = new_ptr;
	tc->flags &= ~TALLOC_FLAG_FREE; 
	if (tc->parent) {
		tc->parent->child = new_ptr;
	}
	if (tc->child) {
		tc->child->parent = new_ptr;
	}

	if (tc->prev) {
		tc->prev->next = tc;
	}
	if (tc->next) {
		tc->next->prev = tc;
	}

	tc->size = size;
	talloc_set_name_const(TC_PTR_FROM_CHUNK(tc), name);

	return TC_PTR_FROM_CHUNK(tc);
}

/* 
   move a lump of memory from one talloc context to another return the
   ptr on success, or NULL if it could not be transferred.
   passing NULL as ptr will always return NULL with no side effects.
*/
void *talloc_steal(const void *new_ctx, const void *ptr)
{
	struct talloc_chunk *tc, *new_tc;

	if (!ptr) {
		return NULL;
	}

	if (new_ctx == NULL) {
		new_ctx = null_context;
	}

	tc = talloc_chunk_from_ptr(ptr);

	if (new_ctx == NULL) {
		if (tc->parent) {
			_TLIST_REMOVE(tc->parent->child, tc);
			if (tc->parent->child) {
				tc->parent->child->parent = tc->parent;
			}
		} else {
			if (tc->prev) tc->prev->next = tc->next;
			if (tc->next) tc->next->prev = tc->prev;
		}
		
		tc->parent = tc->next = tc->prev = NULL;
		return discard_const_p(void, ptr);
	}

	new_tc = talloc_chunk_from_ptr(new_ctx);

	if (tc == new_tc || tc->parent == new_tc) {
		return discard_const_p(void, ptr);
	}

	if (tc->parent) {
		_TLIST_REMOVE(tc->parent->child, tc);
		if (tc->parent->child) {
			tc->parent->child->parent = tc->parent;
		}
	} else {
		if (tc->prev) tc->prev->next = tc->next;
		if (tc->next) tc->next->prev = tc->prev;
	}

	tc->parent = new_tc;
	if (new_tc->child) new_tc->child->parent = NULL;
	_TLIST_ADD(new_tc->child, tc);

	return discard_const_p(void, ptr);
}

/*
  return the total size of a talloc pool (subtree)
*/
long talloc_total_size(const void *ptr)
{
	long total = 0;
	struct talloc_chunk *c, *tc;
	
	if (ptr == NULL) {
		ptr = null_context;
	}
	if (ptr == NULL) {
		return 0;
	}

	tc = talloc_chunk_from_ptr(ptr);

	if (tc->flags & TALLOC_FLAG_LOOP) {
		return 0;
	}

	tc->flags |= TALLOC_FLAG_LOOP;

	total = tc->size;
	for (c=tc->child;c;c=c->next) {
		total += talloc_total_size(TC_PTR_FROM_CHUNK(c));
	}

	tc->flags &= ~TALLOC_FLAG_LOOP;

	return total;
}

/*
  return the total number of blocks in a talloc pool (subtree)
*/
long talloc_total_blocks(const void *ptr)
{
	long total = 0;
	struct talloc_chunk *c, *tc = talloc_chunk_from_ptr(ptr);

	if (tc->flags & TALLOC_FLAG_LOOP) {
		return 0;
	}

	tc->flags |= TALLOC_FLAG_LOOP;

	total++;
	for (c=tc->child;c;c=c->next) {
		total += talloc_total_blocks(TC_PTR_FROM_CHUNK(c));
	}

	tc->flags &= ~TALLOC_FLAG_LOOP;

	return total;
}

/*
  return the number of external references to a pointer
*/
static int talloc_reference_count(const void *ptr)
{
	struct talloc_chunk *tc = talloc_chunk_from_ptr(ptr);
	struct talloc_reference_handle *h;
	int ret = 0;

	for (h=tc->refs;h;h=h->next) {
		ret++;
	}
	return ret;
}

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
void talloc_report_depth(const void *ptr, FILE *f, int depth)
{
	struct talloc_chunk *c, *tc = talloc_chunk_from_ptr(ptr);

	if (tc->flags & TALLOC_FLAG_LOOP) {
		return;
	}

	tc->flags |= TALLOC_FLAG_LOOP;

	for (c=tc->child;c;c=c->next) {
		if (c->name == TALLOC_MAGIC_REFERENCE) {
			struct talloc_reference_handle *handle = TC_PTR_FROM_CHUNK(c);
			const char *name2 = talloc_get_name(handle->ptr);
			fprintf(f, "%*sreference to: %s\n", depth*4, "", name2);
		} else {
			const char *name = talloc_get_name(TC_PTR_FROM_CHUNK(c));
			fprintf(f, "%*s%-30s contains %6lu bytes in %3lu blocks (ref %d)\n", 
				depth*4, "",
				name,
				(unsigned long)talloc_total_size(TC_PTR_FROM_CHUNK(c)),
				(unsigned long)talloc_total_blocks(TC_PTR_FROM_CHUNK(c)),
				talloc_reference_count(TC_PTR_FROM_CHUNK(c)));
			talloc_report_depth(TC_PTR_FROM_CHUNK(c), f, depth+1);
		}
	}
	tc->flags &= ~TALLOC_FLAG_LOOP;
}

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
void talloc_report_full(const void *ptr, FILE *f)
{
	if (ptr == NULL) {
		ptr = null_context;
	}
	if (ptr == NULL) return;

	fprintf(f,"full talloc report on '%s' (total %lu bytes in %lu blocks)\n", 
		talloc_get_name(ptr), 
		(unsigned long)talloc_total_size(ptr),
		(unsigned long)talloc_total_blocks(ptr));

	talloc_report_depth(ptr, f, 1);
	fflush(f);
}

/*
  report on memory usage by all children of a pointer
*/
void talloc_report(const void *ptr, FILE *f)
{
	struct talloc_chunk *c, *tc;

	if (ptr == NULL) {
		ptr = null_context;
	}
	if (ptr == NULL) return;
       
	fprintf(f,"talloc report on '%s' (total %lu bytes in %lu blocks)\n", 
		talloc_get_name(ptr), 
		(unsigned long)talloc_total_size(ptr),
		(unsigned long)talloc_total_blocks(ptr));

	tc = talloc_chunk_from_ptr(ptr);

	for (c=tc->child;c;c=c->next) {
		fprintf(f, "\t%-30s contains %6lu bytes in %3lu blocks\n", 
			talloc_get_name(TC_PTR_FROM_CHUNK(c)),
			(unsigned long)talloc_total_size(TC_PTR_FROM_CHUNK(c)),
			(unsigned long)talloc_total_blocks(TC_PTR_FROM_CHUNK(c)));
	}
	fflush(f);
}

/*
  report on any memory hanging off the null context
*/
static void talloc_report_null(void)
{
	if (talloc_total_size(null_context) != 0) {
		talloc_report(null_context, stderr);
	}
}

/*
  report on any memory hanging off the null context
*/
static void talloc_report_null_full(void)
{
	if (talloc_total_size(null_context) != 0) {
		talloc_report_full(null_context, stderr);
	}
}

/*
 free allocated global memory
*/

void talloc_nc_free(void)
{
	if ( null_context )
		talloc_free( (void*)null_context ); 
}

/*
  enable tracking of the NULL context
*/
void talloc_enable_null_tracking(void)
{
	if (null_context == NULL) {
		null_context = talloc_named_const(NULL, 0, "null_context");
	}
}

#ifdef _SAMBA_BUILD_
/* Ugly calls to Samba-specific sprintf_append... JRA. */

/*
  report on memory usage by all children of a pointer, giving a full tree view
*/
static void talloc_report_depth_str(const void *ptr, char **pps, ssize_t *plen, size_t *pbuflen, int depth)
{
	struct talloc_chunk *c, *tc = talloc_chunk_from_ptr(ptr);

	if (tc->flags & TALLOC_FLAG_LOOP) {
		return;
	}

	tc->flags |= TALLOC_FLAG_LOOP;

	for (c=tc->child;c;c=c->next) {
		if (c->name == TALLOC_MAGIC_REFERENCE) {
			struct talloc_reference_handle *handle = TC_PTR_FROM_CHUNK(c);
			const char *name2 = talloc_get_name(handle->ptr);

			sprintf_append(NULL, pps, plen, pbuflen,
				"%*sreference to: %s\n", depth*4, "", name2);

		} else {
			const char *name = talloc_get_name(TC_PTR_FROM_CHUNK(c));

			sprintf_append(NULL, pps, plen, pbuflen,
				"%*s%-30s contains %6lu bytes in %3lu blocks (ref %d)\n", 
				depth*4, "",
				name,
				(unsigned long)talloc_total_size(TC_PTR_FROM_CHUNK(c)),
				(unsigned long)talloc_total_blocks(TC_PTR_FROM_CHUNK(c)),
				talloc_reference_count(TC_PTR_FROM_CHUNK(c)));

			talloc_report_depth_str(TC_PTR_FROM_CHUNK(c), pps, plen, pbuflen, depth+1);
		}
	}
	tc->flags &= ~TALLOC_FLAG_LOOP;
}

/*
  report on memory usage by all children of a pointer
*/
char *talloc_describe_all(void)
{
	ssize_t len = 0;
	size_t buflen = 512;
	char *s = NULL;

	if (null_context == NULL) {
		return NULL;
	}

	sprintf_append(NULL, &s, &len, &buflen,
		"full talloc report on '%s' (total %lu bytes in %lu blocks)\n", 
		talloc_get_name(null_context), 
		(unsigned long)talloc_total_size(null_context),
		(unsigned long)talloc_total_blocks(null_context));

	if (!s) {
		return NULL;
	}
	talloc_report_depth_str(null_context, &s, &len, &buflen, 1);
	return s;
}
#endif

/*
  enable leak reporting on exit
*/
void talloc_enable_leak_report(void)
{
	talloc_enable_null_tracking();
	atexit(talloc_report_null);
}

/*
  enable full leak reporting on exit
*/
void talloc_enable_leak_report_full(void)
{
	talloc_enable_null_tracking();
	atexit(talloc_report_null_full);
}

/* 
   talloc and zero memory. 
*/
void *_talloc_zero(const void *ctx, size_t size, const char *name)
{
	void *p = talloc_named_const(ctx, size, name);

	if (p) {
		memset(p, '\0', size);
	}

	return p;
}


/*
  memdup with a talloc. 
*/
void *_talloc_memdup(const void *t, const void *p, size_t size, const char *name)
{
	void *newp = talloc_named_const(t, size, name);

	if (newp) {
		memcpy(newp, p, size);
	}

	return newp;
}

/*
  strdup with a talloc 
*/
char *talloc_strdup(const void *t, const char *p)
{
	char *ret;
	if (!p) {
		return NULL;
	}
	ret = talloc_memdup(t, p, strlen(p) + 1);
	if (ret) {
		talloc_set_name_const(ret, ret);
	}
	return ret;
}

/*
 append to a talloced string 
*/
char *talloc_append_string(const void *t, char *orig, const char *append)
{
	char *ret;
	size_t olen = strlen(orig);
	size_t alenz;

	if (!append)
		return orig;

	alenz = strlen(append) + 1;

	ret = talloc_realloc(t, orig, char, olen + alenz);
	if (!ret)
		return NULL;

	/* append the string with the trailing \0 */
	memcpy(&ret[olen], append, alenz);

	return ret;
}

/*
  strndup with a talloc 
*/
char *talloc_strndup(const void *t, const char *p, size_t n)
{
	size_t len;
	char *ret;

	for (len=0; len<n && p[len]; len++) ;

	ret = _talloc(t, len + 1);
	if (!ret) { return NULL; }
	memcpy(ret, p, len);
	ret[len] = 0;
	talloc_set_name_const(ret, ret);
	return ret;
}

#ifndef VA_COPY
#ifdef HAVE_VA_COPY
#define VA_COPY(dest, src) va_copy(dest, src)
#elif defined(HAVE___VA_COPY)
#define VA_COPY(dest, src) __va_copy(dest, src)
#else
#define VA_COPY(dest, src) (dest) = (src)
#endif
#endif

char *talloc_vasprintf(const void *t, const char *fmt, va_list ap)
{	
	int len;
	char *ret;
	va_list ap2;
	char c;
	
	VA_COPY(ap2, ap);

	/* this call looks strange, but it makes it work on older solaris boxes */
	if ((len = vsnprintf(&c, 1, fmt, ap2)) < 0) {
		return NULL;
	}

	ret = _talloc(t, len+1);
	if (ret) {
		VA_COPY(ap2, ap);
		vsnprintf(ret, len+1, fmt, ap2);
		talloc_set_name_const(ret, ret);
	}

	return ret;
}


/*
  Perform string formatting, and return a pointer to newly allocated
  memory holding the result, inside a memory pool.
 */
char *talloc_asprintf(const void *t, const char *fmt, ...)
{
	va_list ap;
	char *ret;

	va_start(ap, fmt);
	ret = talloc_vasprintf(t, fmt, ap);
	va_end(ap);
	return ret;
}


/**
 * Realloc @p s to append the formatted result of @p fmt and @p ap,
 * and return @p s, which may have moved.  Good for gradually
 * accumulating output into a string buffer.
 **/

static char *talloc_vasprintf_append(char *s, const char *fmt, va_list ap) PRINTF_ATTRIBUTE(2,0);

static char *talloc_vasprintf_append(char *s, const char *fmt, va_list ap)
{	
	struct talloc_chunk *tc;
	int len, s_len;
	va_list ap2;

	if (s == NULL) {
		return talloc_vasprintf(NULL, fmt, ap);
	}

	tc = talloc_chunk_from_ptr(s);

	VA_COPY(ap2, ap);

	s_len = tc->size - 1;
	if ((len = vsnprintf(NULL, 0, fmt, ap2)) <= 0) {
		/* Either the vsnprintf failed or the format resulted in
		 * no characters being formatted. In the former case, we
		 * ought to return NULL, in the latter we ought to return
		 * the original string. Most current callers of this 
		 * function expect it to never return NULL.
		 */
		return s;
	}

	s = talloc_realloc(NULL, s, char, s_len + len+1);
	if (!s) return NULL;

	VA_COPY(ap2, ap);

	vsnprintf(s+s_len, len+1, fmt, ap2);
	talloc_set_name_const(s, s);

	return s;
}

/*
  Realloc @p s to append the formatted result of @p fmt and return @p
  s, which may have moved.  Good for gradually accumulating output
  into a string buffer.
 */
char *talloc_asprintf_append(char *s, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	s = talloc_vasprintf_append(s, fmt, ap);
	va_end(ap);
	return s;
}

/*
  alloc an array, checking for integer overflow in the array size
*/
void *_talloc_array(const void *ctx, size_t el_size, unsigned count, const char *name)
{
	if (count >= MAX_TALLOC_SIZE/el_size) {
		return NULL;
	}
	return talloc_named_const(ctx, el_size * count, name);
}

/*
  alloc an zero array, checking for integer overflow in the array size
*/
void *_talloc_zero_array(const void *ctx, size_t el_size, unsigned count, const char *name)
{
	if (count >= MAX_TALLOC_SIZE/el_size) {
		return NULL;
	}
	return _talloc_zero(ctx, el_size * count, name);
}


/*
  realloc an array, checking for integer overflow in the array size
*/
void *_talloc_realloc_array(const void *ctx, void *ptr, size_t el_size, unsigned count, const char *name)
{
	if (count >= MAX_TALLOC_SIZE/el_size) {
		return NULL;
	}
	return _talloc_realloc(ctx, ptr, el_size * count, name);
}

/*
  a function version of talloc_realloc(), so it can be passed as a function pointer
  to libraries that want a realloc function (a realloc function encapsulates
  all the basic capabilities of an allocation library, which is why this is useful)
*/
void *talloc_realloc_fn(const void *context, void *ptr, size_t size)
{
	return _talloc_realloc(context, ptr, size, NULL);
}


static void talloc_autofree(void)
{
	talloc_free(cleanup_context);
	cleanup_context = NULL;
}

/*
  return a context which will be auto-freed on exit
  this is useful for reducing the noise in leak reports
*/
void *talloc_autofree_context(void)
{
	if (cleanup_context == NULL) {
		cleanup_context = talloc_named_const(NULL, 0, "autofree_context");
		atexit(talloc_autofree);
	}
	return cleanup_context;
}

size_t talloc_get_size(const void *context)
{
	struct talloc_chunk *tc;

	if (context == NULL)
		return 0;

	tc = talloc_chunk_from_ptr(context);

	return tc->size;
}

/*
  find a parent of this context that has the given name, if any
*/
void *talloc_find_parent_byname(const void *context, const char *name)
{
	struct talloc_chunk *tc;

	if (context == NULL) {
		return NULL;
	}

	tc = talloc_chunk_from_ptr(context);
	while (tc) {
		if (tc->name && strcmp(tc->name, name) == 0) {
			return TC_PTR_FROM_CHUNK(tc);
		}
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}

	}
	return NULL;
}

/*
  show the parentage of a context
*/
void talloc_show_parents(const void *context, FILE *file)
{
	struct talloc_chunk *tc;

	if (context == NULL) {
		fprintf(file, "talloc no parents for NULL\n");
		return;
	}

	tc = talloc_chunk_from_ptr(context);
	fprintf(file, "talloc parents of '%s'\n", talloc_get_name(context));
	while (tc) {
		fprintf(file, "\t'%s'\n", talloc_get_name(TC_PTR_FROM_CHUNK(tc)));
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}
	}
}

/*
  return 1 if ptr is a parent of context
*/
int talloc_is_parent(const void *context, const char *ptr)
{
	struct talloc_chunk *tc;

	if (context == NULL) {
		return 0;
	}

	tc = talloc_chunk_from_ptr(context);
	while (tc) {
		if (TC_PTR_FROM_CHUNK(tc) == ptr) return 1;
		while (tc && tc->prev) tc = tc->prev;
		if (tc) {
			tc = tc->parent;
		}
	}
	return 0;
}
