/*
 *  Copyright (C) 2005-2006, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * alloc.c -   Memory management functions.  The structures returned from
 *             librefmem APIs are actually pointers to reference counted
 *             blocks of memory.  The functions provided here handle allocating
 *             these blocks (strictly internally to the library), placing
 *             holds on these blocks (publicly) and releasing holds (publicly).
 *
 *             All pointer type return values, including strings are reference
 *             counted.
 *
 *       NOTE: Since reference counted pointers are used to move
 *             these structures around, it is strictly forbidden to
 *             modify the contents of a structure once its pointer has
 *             been returned to a callerthrough an API function.  This
 *             means that all API functions that modify the contents
 *             of a structure must copy the structure, modify the
 *             copy, and return a pointer to the copy.  It is safe to
 *             copy pointers (as long as you hold them) everyone
 *             follows this simple rule.  There is no need for deep
 *             copying of any structure.
 */
#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include <refmem/refmem.h>
#include <refmem/atomic.h>
#include <refmem_local.h>

#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#include <assert.h>
#define ALLOC_MAGIC 0xef37a45d
#define GUARD_MAGIC 0xe3
#endif /* DEBUG */

/* Disable optimization on OSX ppc
   Compilation fails in release mode both with Apple gcc build 5490 and 5493 */
#if defined(__APPLE__) && defined(__ppc__)
#pragma GCC optimization_level 0
#endif

/*
 * struct refcounter
 *
 * Scope: PRIVATE (local to this module)
 *
 * Description:
 *
 * The structure used to manage references.  One of these is prepended to every
 * allocation.  It contains two key pieces of information:
 *
 * - The reference count on the structure or string attached to it
 *
 * - A pointer to a 'destroy' function (a destructor) to be called when
 *   the last reference is released.  This function facilitates tearing down
 *   of any complex structures contained in the reference counted block.  If
 *   it is NULL, no function is called.
 *
 * NOTE: Make sure this has a word aligned length, as it will be placed
 *       before each allocation and will affect the alignment of pointers.
 */
typedef struct refcounter {
#ifdef DEBUG
	unsigned int magic;
	struct refcounter *next;
	const char *file;
	const char *func;
	int line;
#endif /* DEBUG */
	mvp_atomic_t refcount;
	size_t length;
	ref_destroy_t destroy;
} refcounter_t;

#ifdef DEBUG
typedef struct {
	unsigned char magic;
} guard_t;
#endif /* DEBUG */

#define REF_REFCNT(p) ((refcounter_t *)(((unsigned char *)(p)) - sizeof(refcounter_t)))
#define REF_DATA(r) (((unsigned char *)(r)) + sizeof(refcounter_t))

#if defined(DEBUG)
#define REF_ALLOC_BINS	101
static refcounter_t *ref_list[REF_ALLOC_BINS];
#endif /* DEBUG */

#if defined(DEBUG)
static inline void
ref_remove(refcounter_t *ref)
{
	int bin;
	refcounter_t *r, *p;

	bin = ((uintptr_t)ref >> 2) % REF_ALLOC_BINS;

	r = ref_list[bin];
	p = NULL;

	while (r && (r != ref)) {
		p = r;
		r = r->next;
	}

	assert(r == ref);

	if (p) {
		p->next = r->next;
	} else {
		ref_list[bin] = r->next;
	}
}

static inline void
ref_add(refcounter_t *ref)
{
	int bin;

	bin = ((uintptr_t)ref >> 2) % REF_ALLOC_BINS;

	ref->next = ref_list[bin];
	ref_list[bin] = ref;
}

static struct alloc_type {
	const char *file;
	const char *func;
	int line;
	int count;
} alloc_list[128];

void
ref_alloc_show(void)
{
	int i, j;
	int types = 0, bytes = 0, count = 0;
	refcounter_t *r;

	for (i=0; i<REF_ALLOC_BINS; i++) {
		r = ref_list[i];

		while (r) {
			for (j=0; (j<types) && (j<128); j++) {
				if ((alloc_list[j].file == r->file) &&
				    (alloc_list[j].func == r->func) &&
				    (alloc_list[j].line == r->line)) {
					alloc_list[j].count++;
					break;
				}
			}
			if (j == types) {
				alloc_list[j].file = r->file;
				alloc_list[j].func = r->func;
				alloc_list[j].line = r->line;
				alloc_list[j].count = 1;
				types++;
			}
			bytes += r->length + sizeof(*r);
			count++;
			r = r->next;
		}
	}

	printf("refmem allocation count: %d\n", count);
	printf("refmem allocation bytes: %d\n", bytes);
	printf("refmem unique allocation types: %d\n", types);
	for (i=0; i<types; i++) {
		printf("ALLOC: %s %s():%d  count %d\n",
		       alloc_list[i].file, alloc_list[i].func,
		       alloc_list[i].line, alloc_list[i].count);
	}
}
#else
void
ref_alloc_show(void)
{
}
#endif /* DEBUG */

/*
 * ref_alloc(size_t len)
 * 
 * Scope: PRIVATE (mapped to __ref_alloc)
 *
 * Description
 *
 * Allocate a reference counted block of data.
 *
 * Return Value:
 *
 * Success: A non-NULL pointer to  a block of memory at least 'len' bytes long
 *          and safely aligned.  The block is reference counted and can be
 *          released using ref_release().
 *
 * Failure: A NULL pointer.
 */
void *
__ref_alloc(size_t len, const char *file, const char *func, int line)
{
#ifdef DEBUG
	void *block = malloc(sizeof(refcounter_t) + len + sizeof(guard_t));
	guard_t *guard;
#else
	void *block = malloc(sizeof(refcounter_t) + len);
#endif /* DEBUG */
	void *ret = REF_DATA(block);
	refcounter_t *ref = (refcounter_t *)block;

	refmem_dbg(REF_DBG_DEBUG, "%s(%d, ret = %p, ref = %p) {\n",
		   __FUNCTION__, len, ret, ref);
	if (block) {
		memset(block, 0, sizeof(refcounter_t) + len);
		mvp_atomic_set(&ref->refcount, 1);
#ifdef DEBUG
		ref->magic = ALLOC_MAGIC;
		ref->file = file;
		ref->func = func;
		ref->line = line;
		guard = (guard_t*)((uintptr_t)block +
				   sizeof(refcounter_t) + len);
		guard->magic = GUARD_MAGIC;
		ref_add(ref);
#endif /* DEBUG */
		ref->destroy = NULL;
		ref->length = len;
		refmem_dbg(REF_DBG_DEBUG, "%s(%d, ret = %p, ref = %p) }\n",
			   __FUNCTION__, len, ret, ref);
		return ret;
	}
	refmem_dbg(REF_DBG_DEBUG, "%s(%d, ret = %p, ref = %p) !}\n",
		   __FUNCTION__, len, ret, ref);
	return NULL;
}

/*
 * ref_realloc(void *p, size_t len)
 * 
 * Scope: PRIVATE (mapped to __ref_realloc)
 *
 * Description
 *
 * Change the allocation size of a reference counted allocation.
 *
 * Return Value:
 *
 * Success: A non-NULL pointer to  a block of memory at least 'len' bytes long
 *          and safely aligned.  The block is reference counted and can be
 *          released using ref_release().
 *
 * Failure: A NULL pointer.
 */
void *
ref_realloc(void *p, size_t len)
{
	refcounter_t *ref = REF_REFCNT(p);
	void *ret = ref_alloc(len);

	refmem_dbg(REF_DBG_DEBUG, "%s(%d, ret = %p, ref = %p) {\n",
		   __FUNCTION__, len, ret, ref);
#ifdef DEBUG
  if(p)
	  assert(ref->magic == ALLOC_MAGIC);
#endif /* DEBUG */
	if (p && ret) {
		memcpy(ret, p, ref->length);
		ref_set_destroy(ret, ref->destroy);
	}
	if (p) {
		ref_release(p);
	}
	refmem_dbg(REF_DBG_DEBUG, "%s(%d, ret = %p, ref = %p) }\n",
		   __FUNCTION__, len, ret, ref);
	return ret;
}

/*
 * ref_set_destroy(void *block, ref_destroy_t func)
 * 
 * Scope: PRIVATE (mapped to __ref_set_destroy)
 *
 * Description
 *
 * Set the destroy function for a block of data.  The first argument
 * is a pointer to the data block (as returned by ref_alloc()).  The
 * second argument is a pointer to the destroy function which, when
 * called, will be passed one argument, the pointer to the block (as
 * returned by ref_alloc()).  The destroy function is
 * respsonsible for any cleanup needed prior to finally releasing the
 * memory holding the memory block.
 *
 * Return Value: NONE
 */
void
ref_set_destroy(void *data, ref_destroy_t func)
{
	void *block = REF_REFCNT(data);
	refcounter_t *ref = block;

	refmem_dbg(REF_DBG_DEBUG, "%s(%p, func = %p, ref = %p) {\n",
		   __FUNCTION__, data, func, ref);
#ifdef DEBUG
	assert(ref->magic == ALLOC_MAGIC);
#endif /* DEBUG */
	if (data) {
		ref->destroy = func;
	}
	refmem_dbg(REF_DBG_DEBUG, "%s(%p, func = %p, ref = %p) }\n",
		   __FUNCTION__, data, func, ref);
}

/*
 * ref_strdup(char *str)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Similar to the libc version of strdup() except that it returns a pointer
 * to a reference counted string.
 *
 * Return Value: 
 *
 * Success: A non-NULL pointer to  a reference counted string which can be
 *          released using ref_release().
 *
 * Failure: A NULL pointer.
 */
char *
ref_strdup(char *str)
{
	size_t len;
	char *ret = NULL;

	refmem_dbg(REF_DBG_DEBUG, "%s(%p) {\n",
		   __FUNCTION__, str);
	if (str) {
		len = strlen(str) + 1;
		ret = ref_alloc(len);
		if (ret) {
			strncpy(ret, str, len);
			ret[len - 1] = '\0';
		}
		refmem_dbg(REF_DBG_DEBUG,
			   "%s str = %p[%s], len = %d, ret =%p\n",
			   __FUNCTION__, str, str, len, ret);
	}
	refmem_dbg(REF_DBG_DEBUG, "%s() }\n", __FUNCTION__);
	return ret;
}

/*
 * ref_hold(void *p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * This is how holders of references to reference counted blocks take
 * additional references.  The argument is a pointer to a structure or
 * string returned from ref_alloc.  The structure's reference count
 * will be incremented and a pointer to that space returned.
 *
 * There is really  no error condition possible, but if a NULL pointer
 * is passed in, a NULL is returned.
 *
 * NOTE: since this function operates outside of the space that is directly
 *       accessed by  the pointer, if a pointer that was NOT allocated by
 *       ref_alloc() is provided, negative consequences are likely.
 *
 * Return Value: A  pointer to the held space
 */
void *
ref_hold(void *p)
{
	void *block = REF_REFCNT(p);
	refcounter_t *ref = block;
#ifdef DEBUG
	guard_t *guard;
#endif /* DEBUG */

	refmem_dbg(REF_DBG_DEBUG, "%s(%p) {\n", __FUNCTION__, p);
	if (p) {
#ifdef DEBUG
		assert(ref->magic == ALLOC_MAGIC);
		guard = (guard_t*)((uintptr_t)block +
				   sizeof(refcounter_t) + ref->length);
		assert(guard->magic == GUARD_MAGIC);
#endif /* DEBUG */
		mvp_atomic_inc(&ref->refcount);
	}
	refmem_dbg(REF_DBG_DEBUG, "%s(%p) }\n", __FUNCTION__, p);
        return p;
}

/*
 * ref_release(void *p)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * This is how holders of references to reference counted blocks release
 * those references.  The argument is a pointer to a structure or string
 * returned from a librefmem API function (or from ref_alloc).  The
 * structure's reference count will be decremented and, when it reaches zero
 * the structure's destroy function (if any) will be called and then the
 * memory block will be released.
 *
 * Return Value: NONE
 */
void
ref_release(void *p)
{
	void *block = REF_REFCNT(p);
	refcounter_t *ref = block;
#ifdef DEBUG
	guard_t *guard;
#endif /* DEBUG */

	refmem_dbg(REF_DBG_DEBUG, "%s(%p) {\n", __FUNCTION__, p);
	if (p) {
		refmem_dbg(REF_DBG_DEBUG,
			   "%s:%d %s(%p,ref = %p,refcount = %p,length = %d)\n",
			   __FILE__, __LINE__, __FUNCTION__,
			   p, ref, ref->refcount, ref->length);
#ifdef DEBUG
		assert(ref->magic == ALLOC_MAGIC);
		guard = (guard_t*)((uintptr_t)block +
				   sizeof(refcounter_t) + ref->length);
		assert(guard->magic == GUARD_MAGIC);
#endif /* DEBUG */
		if (mvp_atomic_dec_and_test(&ref->refcount)) {
			/*
			 * Last reference, destroy the structure (if
			 * there is a destroy function) and free the
			 * block.
			 */
			if (ref->destroy) {
				ref->destroy(p);
			}
			refmem_dbg(REF_DBG_DEBUG,
				   "%s:%d %s() -- free it\n",
				   __FILE__, __LINE__, __FUNCTION__);
#ifdef DEBUG
			ref->magic = 0;
			guard->magic = 0;
#ifndef _WIN32
			refmem_ref_remove(ref);
#endif
			ref->next = NULL;
#endif /* DEBUG */
			free(block);
		}
	}
	refmem_dbg(REF_DBG_DEBUG, "%s(%p) }\n", __FUNCTION__, p);
}

#if defined(__APPLE__) && defined(__ppc__)
#pragma GCC optimization_level reset
#endif
