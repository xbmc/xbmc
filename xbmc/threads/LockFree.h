/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LOCK_FREE_H__
#define __LOCK_FREE_H__

#include <cstring>
#include "Atomics.h"

#define SPINLOCK_ACQUIRE(l) while(cas(&l, 0, 1)) {}
#define SPINLOCK_RELEASE(l) l = 0

// A unique-valued pointer. Version is incremented with each write.
union atomic_ptr
{
#if !defined(__ppc__) && !defined(__powerpc__) && !defined(__arm__)
  long long d;
  struct {
    void* ptr;
    long version;
  };
#else
  long d;
  struct {
    void* ptr;
  };
#endif
};

#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
  #define atomic_ptr_to_long(p) (long) *((long*)&p)
#else
  // This is ugly but correct as long as sizeof(void*) == sizeof(long)...
  #define atomic_ptr_to_long_long(p) (long long) *((long long*)&p)
#endif

struct lf_node
{
  atomic_ptr next;
};

///////////////////////////////////////////////////////////////////////////
// Fast Stack
///////////////////////////////////////////////////////////////////////////


struct lf_stack
{
  atomic_ptr top;
  long count;
};


void lf_stack_init(lf_stack* pStack);
void lf_stack_push(lf_stack* pStack, lf_node* pNode);
lf_node* lf_stack_pop(lf_stack* pStack);

///////////////////////////////////////////////////////////////////////////
// Fast Heap for Fixed-size Blocks
///////////////////////////////////////////////////////////////////////////
struct lf_heap_chunk
{
  lf_heap_chunk* next;
  long size;
};

struct lf_heap
{
  lf_stack free_list;
  long alloc_lock;
  lf_heap_chunk* top_chunk;
  long block_size;
};


void lf_heap_init(lf_heap* pHeap, size_t blockSize, size_t initialSize = 0);
void lf_heap_grow(lf_heap* pHeap, size_t size = 0);
void lf_heap_deinit(lf_heap* pHeap);
void* lf_heap_alloc(lf_heap* pHeap);
void lf_heap_free(lf_heap* pHeap, void* p);

///////////////////////////////////////////////////////////////////////////
// Lock-free queue
///////////////////////////////////////////////////////////////////////////
struct lf_queue_node
{
  atomic_ptr next;
  void* value; // TODO: Convert to a template
};

struct lf_queue
{
  atomic_ptr head;
  atomic_ptr tail;
  lf_heap node_heap;
  long len;
};

#define lf_queue_new_node(q) (lf_queue_node*)lf_heap_alloc(&q->node_heap)
#define lf_queue_free_node(q,n) lf_heap_free(&q->node_heap, n)

void lf_queue_init(lf_queue* pQueue);
void lf_queue_deinit(lf_queue* pQueue);
void lf_queue_enqueue(lf_queue* pQueue, void* pVal);
void* lf_queue_dequeue(lf_queue* pQueue);

#endif
