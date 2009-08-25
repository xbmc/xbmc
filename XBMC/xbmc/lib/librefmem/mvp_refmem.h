/*
 *  Copyright (C) 2004-2006, Eric Lund, Jon Gettler
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

/** \file refmem.h
 * A C library for managing reference counted memory allocations
 */

#ifndef __REFMEM_H
#define __REFMEM_H

#include <mvp_atomic.h>
#include <mvp_debug.h>

/*
 * -----------------------------------------------------------------
 * Types
 * -----------------------------------------------------------------
 */

/* Return current number of references outstanding for everything */
extern int refmem_get_refcount();

/**
 * Release a reference to allocated memory.
 * \param p allocated memory
 */
extern void refmem_release(void *p);

/**
 * Add a reference to allocated memory.
 * \param p allocated memory
 * \return new reference
 */
extern void *refmem_hold(void *p);

/**
 * Duplicate a string using reference counted memory.
 * \param str string to duplicate
 * \return reference counted string
 */
extern char *refmem_strdup(char *str);

/**
 * Allocate reference counted memory. (PRIVATE)
 * \param len allocation size
 * \param file filename
 * \param func function name
 * \param line line number
 * \return reference counted memory
 */
extern void *__refmem_alloc(size_t len,
				 const char *file,
				 const char *func,
				 int line);

/**
 * Allocate a block of reference counted memory.
 * \param len allocation size
 * \return pointer to reference counted memory block
 */
#if defined(DEBUG)
#define refmem_alloc(l) (__refmem_alloc((l), __FILE__, __FUNC__, __LINE__))
#else
#define refmem_alloc(l) (__refmem_alloc((l), (char *)0, (char *)0, 0))
#endif
/**
 * Reallocate reference counted memory.
 * \param p allocated memory
 * \param len new allocation size
 * \return reference counted memory
 */
extern void *refmem_realloc(void *p, size_t len);

typedef void (*refmem_destroy_t)(void *p);

/**
 * Add a destroy callback for reference counted memory.
 * \param block allocated memory
 * \param func destroy function
 */
extern void refmem_set_destroy(void *block, refmem_destroy_t func);

/**
 * Print allocation information to stdout.
 */
extern void refmem_alloc_show(void);

#endif /* __REFMEM_H */
