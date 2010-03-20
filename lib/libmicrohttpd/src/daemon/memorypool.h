/*
     This file is part of libmicrohttpd
     (C) 2007, 2009 Daniel Pittman and Christian Grothoff

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file memorypool.h
 * @brief memory pool; mostly used for efficient (de)allocation
 *        for each connection and bounding memory use for each
 *        request
 * @author Christian Grothoff
 */

#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include "internal.h"

/**
 * Opaque handle for a memory pool.
 * Pools are not reentrant and must not be used
 * by multiple threads.
 */
struct MemoryPool;

/**
 * Create a memory pool.
 *
 * @param max maximum size of the pool
 */
struct MemoryPool *MHD_pool_create (size_t max);

/**
 * Destroy a memory pool.
 */
void MHD_pool_destroy (struct MemoryPool *pool);

/**
 * Allocate size bytes from the pool.
 *
 * @param from_end allocate from end of pool (set to MHD_YES);
 *        use this for small, persistent allocations that
 *        will never be reallocated
 * @return NULL if the pool cannot support size more
 *         bytes
 */
void *MHD_pool_allocate (struct MemoryPool *pool,
                         size_t size, int from_end);

/**
 * Reallocate a block of memory obtained from the pool.
 * This is particularly efficient when growing or
 * shrinking the block that was last (re)allocated.
 * If the given block is not the most recenlty
 * (re)allocated block, the memory of the previous
 * allocation may be leaked until the pool is
 * destroyed (and copying the data maybe required).
 *
 * @param old the existing block
 * @param old_size the size of the existing block
 * @param new_size the new size of the block
 * @return new address of the block, or
 *         NULL if the pool cannot support new_size
 *         bytes (old continues to be valid for old_size)
 */
void *MHD_pool_reallocate (struct MemoryPool *pool,
                           void *old,
                           size_t old_size, 
			   size_t new_size);

/**
 * Clear all entries from the memory pool except
 * for "keep" of the given "size".
 *
 * @param keep pointer to the entry to keep (maybe NULL)
 * @param size how many bytes need to be kept at this address
 * @return addr new address of "keep" (if it had to change)
 */
void *MHD_pool_reset (struct MemoryPool *pool,
		      void *keep, 
		      size_t size);

#endif
