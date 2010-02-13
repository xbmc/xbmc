/*
     This file is part of libmicrohttpd
     (C) 2007, 2009, 2010 Daniel Pittman and Christian Grothoff

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
 * @file memorypool.c
 * @brief memory pool
 * @author Christian Grothoff
 */
#include "memorypool.h"

/* define MAP_ANONYMOUS for Mac OS X */
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

/**
 * Align to 2x word size (as GNU libc does).
 */
#define ALIGN_SIZE (2 * sizeof(void*))

/**
 * Round up 'n' to a multiple of ALIGN_SIZE.
 */
#define ROUND_TO_ALIGN(n) ((n+(ALIGN_SIZE-1)) & (~(ALIGN_SIZE-1)))

struct MemoryPool
{

  /**
   * Pointer to the pool's memory
   */
  char *memory;

  /**
   * Size of the pool.
   */
  size_t size;

  /**
   * Offset of the first unallocated byte.
   */
  size_t pos;

  /**
   * Offset of the last unallocated byte.
   */
  size_t end;

  /**
   * MHD_NO if pool was malloc'ed, MHD_YES if mmapped.
   */
  int is_mmap;
};

/**
 * Create a memory pool.
 *
 * @param max maximum size of the pool
 */
struct MemoryPool *
MHD_pool_create (size_t max)
{
  struct MemoryPool *pool;

  pool = malloc (sizeof (struct MemoryPool));
  if (pool == NULL)
    return NULL;
#ifdef MAP_ANONYMOUS
  pool->memory = MMAP (NULL, max, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS, -1, 0);
#else
  pool->memory = MAP_FAILED;
#endif
  if ((pool->memory == MAP_FAILED) || (pool->memory == NULL))
    {
      pool->memory = malloc (max);
      if (pool->memory == NULL)
        {
          free (pool);
          return NULL;
        }
      pool->is_mmap = MHD_NO;
    }
  else
    {
      pool->is_mmap = MHD_YES;
    }
  pool->pos = 0;
  pool->end = max;
  pool->size = max;
  return pool;
}

/**
 * Destroy a memory pool.
 */
void
MHD_pool_destroy (struct MemoryPool *pool)
{
  if (pool == NULL)
    return;
  if (pool->is_mmap == MHD_NO)
    free (pool->memory);
  else
    MUNMAP (pool->memory, pool->size);
  free (pool);
}

/**
 * Allocate size bytes from the pool.
 * @return NULL if the pool cannot support size more
 *         bytes
 */
void *
MHD_pool_allocate (struct MemoryPool *pool, 
		   size_t size, int from_end)
{
  void *ret;

  size = ROUND_TO_ALIGN (size);
  if ((pool->pos + size > pool->end) || (pool->pos + size < pool->pos))
    return NULL;
  if (from_end == MHD_YES)
    {
      ret = &pool->memory[pool->end - size];
      pool->end -= size;
    }
  else
    {
      ret = &pool->memory[pool->pos];
      pool->pos += size;
    }
  return ret;
}

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
void *
MHD_pool_reallocate (struct MemoryPool *pool,
                     void *old, 
		     size_t old_size, 
		     size_t new_size)
{
  void *ret;

  new_size = ROUND_TO_ALIGN (new_size);
  if ((pool->end < old_size) || (pool->end < new_size))
    return NULL;                /* unsatisfiable or bogus request */

  if ((pool->pos >= old_size) && (&pool->memory[pool->pos - old_size] == old))
    {
      /* was the previous allocation - optimize! */
      if (pool->pos + new_size - old_size <= pool->end)
        {
          /* fits */
          pool->pos += new_size - old_size;
          if (new_size < old_size)      /* shrinking - zero again! */
            memset (&pool->memory[pool->pos], 0, old_size - new_size);
          return old;
        }
      /* does not fit */
      return NULL;
    }
  if (new_size <= old_size)
    return old;                 /* cannot shrink, no need to move */
  if ((pool->pos + new_size >= pool->pos) &&
      (pool->pos + new_size <= pool->end))
    {
      /* fits */
      ret = &pool->memory[pool->pos];
      memcpy (ret, old, old_size);
      pool->pos += new_size;
      return ret;
    }
  /* does not fit */
  return NULL;
}

/**
 * Clear all entries from the memory pool except
 * for "keep" of the given "size".
 *
 * @param keep pointer to the entry to keep (maybe NULL)
 * @param size how many bytes need to be kept at this address
 * @return addr new address of "keep" (if it had to change)
 */
void *
MHD_pool_reset (struct MemoryPool *pool, 
		void *keep, 
		size_t size)
{
  size = ROUND_TO_ALIGN (size);
  if (keep != NULL)
    {
      if (keep != pool->memory)
        {
          memmove (pool->memory, keep, size);
          keep = pool->memory;
        }
      pool->pos = size;
    }
  pool->end = pool->size;
  return keep;
}



/* end of memorypool.c */
