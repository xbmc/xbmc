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
 
#ifdef __ppc__
#pragma GCC optimization_level 0
#endif

#include "LockFree.h"
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////
// Fast stack implementation
// NOTE: non-locking only on systems that support atomic cas2 operations
///////////////////////////////////////////////////////////////////////////
void lf_stack_init(lf_stack* pStack)
{
  pStack->top.ptr = NULL;
  pStack->count = 0;
}

void lf_stack_push(lf_stack* pStack, lf_node* pNode)
{
  atomic_ptr top, newTop;
  do
  {
    top = pStack->top;
    pNode->next.ptr = top.ptr; // Link in the new node
    newTop.ptr = pNode;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
  } while(cas((long*)&pStack->top, atomic_ptr_to_long(top), atomic_ptr_to_long(newTop)) != atomic_ptr_to_long(top));
#else
    newTop.version = top.version + 1;
  } while(cas2((long long*)&pStack->top, atomic_ptr_to_long_long(top), atomic_ptr_to_long_long(newTop)) != atomic_ptr_to_long_long(top));
#endif
  AtomicIncrement(&pStack->count);
}

lf_node* lf_stack_pop(lf_stack* pStack)
{
  atomic_ptr top, newTop;
  do
  {
    top = pStack->top;
    if (top.ptr == NULL)
      return NULL;
    newTop.ptr = ((lf_node*)top.ptr)->next.ptr; // Unlink the current top node
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
  } while(cas((long*)&pStack->top, atomic_ptr_to_long(top), atomic_ptr_to_long(newTop)) != atomic_ptr_to_long(top));
#else
    newTop.version = top.version + 1;
  } while(cas2((long long*)&pStack->top, atomic_ptr_to_long_long(top), atomic_ptr_to_long_long(newTop)) != atomic_ptr_to_long_long(top));
#endif
  AtomicDecrement(&pStack->count);
  return (lf_node*)top.ptr;
}

///////////////////////////////////////////////////////////////////////////
// Fast heap implementation
// NOTE: non-locking only on systems that support atomic cas2 operations
///////////////////////////////////////////////////////////////////////////
// TODO: Implement auto-shrink based on chunk reference counts

// TODO: Read the page size from the OS or allow caller to specify
// Maybe have a minimum number of blocks...
#define MIN_ALLOC 4096

void lf_heap_init(lf_heap* pHeap, size_t blockSize, size_t initialSize /*= 0*/)
{
  pHeap->alloc_lock = 0; // Initialize the allocation lock
  pHeap->top_chunk = NULL;

  lf_stack_init(&pHeap->free_list); // Initialize the free-list stack

  // Perform a few sanity checks on the parameters
  if (blockSize < sizeof(lf_node)) // Make sure we have blocks big enough to store in the free-list
    blockSize = sizeof(lf_node);
  pHeap->block_size = blockSize;

  if (initialSize < 10 * blockSize)
    initialSize = 10 * blockSize; // TODO: This should be more intelligent

  lf_heap_grow(pHeap, initialSize); // Allocate the first chunk
}

void lf_heap_grow(lf_heap* pHeap, size_t size /*= 0*/)
{

  long blockSize = pHeap->block_size; // This has already been checked for sanity
  if (!size || size < MIN_ALLOC - sizeof(lf_heap_chunk)) // Allocate at least one page from the OS (TODO: Try valloc)
    size = MIN_ALLOC - sizeof(lf_heap_chunk);
  unsigned int blockCount = size / blockSize;
  if (size % blockSize) // maxe sure we have complete blocks
    size = blockSize * ++blockCount;

  // Allocate the first chunk from the general heap and link it into the chunk list
  long mallocSize = size +  sizeof(lf_heap_chunk);
  lf_heap_chunk* pChunk = (lf_heap_chunk*) malloc(mallocSize);
  pChunk->size = mallocSize;
  SPINLOCK_ACQUIRE(pHeap->alloc_lock); // Lock the chunk list. Contention here is VERY unlikely, so use the simplest possible sync mechanism.
  pChunk->next = pHeap->top_chunk;
  pHeap->top_chunk = pChunk; // Link it into the list
  SPINLOCK_RELEASE(pHeap->alloc_lock); // The list is now consistent

  // Add all blocks to the free-list
  unsigned char* pBlock = (unsigned char*)pChunk + sizeof(lf_heap_chunk);
  for ( unsigned int block = 0; block < blockCount; block++)
  {
    lf_stack_push(&pHeap->free_list, (lf_node*)pBlock);
    pBlock += blockSize;
  }
}

void lf_heap_deinit(lf_heap* pHeap)
{
  // Free all allocated chunks
  lf_heap_chunk* pNext;
  for(lf_heap_chunk* pChunk = pHeap->top_chunk; pChunk;  pChunk = pNext)
  {
    pNext = pChunk->next;
    free(pChunk);
  }
}

void* lf_heap_alloc(lf_heap* pHeap)
{
  void * p = lf_stack_pop(&pHeap->free_list);
  if (!p)
  {
    lf_heap_grow(pHeap, 0);
    // TODO: should we just call in recursively?
    return lf_stack_pop(&pHeap->free_list); // If growing didn't help, something is wrong (or someone else took them all REALLY fast)
  }
  return p;
}

void lf_heap_free(lf_heap* pHeap, void* p)
{
  if (!p) // Allow for NULL to pass safely
    return;
  lf_stack_push(&pHeap->free_list, (lf_node*)p); // Return the block to the free list
}

///////////////////////////////////////////////////////////////////////////
// Lock-free queue
///////////////////////////////////////////////////////////////////////////
void lf_queue_init(lf_queue* pQueue)
{
  pQueue->len = 0;
  lf_heap_init(&pQueue->node_heap, sizeof(lf_queue_node)); // Intialize the node heap
  lf_queue_node* pNode = lf_queue_new_node(pQueue); // Create the 'empty' node
  pNode->next.ptr = NULL;
  pNode->value = (void*)0xdeadf00d;
  pQueue->head.ptr = pQueue->tail.ptr = pNode;
}

void lf_queue_deinit(lf_queue* pQueue)
{
  lf_heap_deinit(&pQueue->node_heap); // Clean up the node heap
}

// TODO: template-ize
void lf_queue_enqueue(lf_queue* pQueue, void* value)
{
  lf_queue_node* pNode = lf_queue_new_node(pQueue); // Get a container
  pNode->value = value;
  pNode->next.ptr = NULL;
  atomic_ptr tail, next, node;
  do
  {
    tail = pQueue->tail;
    next = ((lf_queue_node*)tail.ptr)->next;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
    if (atomic_ptr_to_long(tail) == atomic_ptr_to_long(pQueue->tail)) // Check consistency
#else
    if (atomic_ptr_to_long_long(tail) == atomic_ptr_to_long_long(pQueue->tail)) // Check consistency
#endif
    {
      if (next.ptr == NULL) // Was tail pointing to the last node?
      {
        node.ptr = pNode;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
        if (cas((long*)&((lf_queue_node*)tail.ptr)->next, atomic_ptr_to_long(next), atomic_ptr_to_long(node)) == atomic_ptr_to_long(next)) // Try to link node at end
#else
        node.version = next.version + 1;
        if (cas2((long long*)&((lf_queue_node*)tail.ptr)->next, atomic_ptr_to_long_long(next), atomic_ptr_to_long_long(node)) == atomic_ptr_to_long_long(next)) // Try to link node at end
#endif
          break; // enqueue is done.
      }
      else // tail was lagging, try to help...
      {
        node.ptr = next.ptr;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
        cas((long*)&pQueue->tail, atomic_ptr_to_long(tail), atomic_ptr_to_long(node)); // We don't care if we  are successful or not
#else
        node.version = tail.version + 1;
        cas2((long long*)&pQueue->tail, atomic_ptr_to_long_long(tail), atomic_ptr_to_long_long(node)); // We don't care if we  are successful or not
#endif
      }
    }
  } while (true); // Keep trying until the enqueue is done
  node.ptr = pNode;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
  cas((long*)&pQueue->tail, atomic_ptr_to_long(tail), atomic_ptr_to_long(node)); // Try to swing the tail to the new node
#else
  node.version = tail.version + 1;
  cas2((long long*)&pQueue->tail, atomic_ptr_to_long_long(tail), atomic_ptr_to_long_long(node)); // Try to swing the tail to the new node
#endif
  AtomicIncrement(&pQueue->len);
}

// TODO: template-ize
void* lf_queue_dequeue(lf_queue* pQueue)
{
  atomic_ptr head, tail, next, node;
  void* pVal = NULL;
  do
  {
    head = pQueue->head;
    tail = pQueue->tail;
    next = ((lf_queue_node*)head.ptr)->next;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
    if (atomic_ptr_to_long(head) == atomic_ptr_to_long(pQueue->head)) // Check consistency
#else
    if (atomic_ptr_to_long_long(head) == atomic_ptr_to_long_long(pQueue->head)) // Check consistency
#endif
    {
      if (head.ptr == tail.ptr) // Queue is empty or tail is lagging
      {
        if (next.ptr == NULL) // Queue is empty
          return NULL;
        node.ptr = next.ptr;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
        cas((long*)&pQueue->tail, atomic_ptr_to_long(tail), atomic_ptr_to_long(node)); // Tail is lagging. Try to advance it.
#else
        node.version = tail.version + 1;
        cas2((long long*)&pQueue->tail, atomic_ptr_to_long_long(tail), atomic_ptr_to_long_long(node)); // Tail is lagging. Try to advance it.
#endif
      }
      else // Tail is consistent. No need to deal with it.
      {
        pVal = ((lf_queue_node*)next.ptr)->value;
        node.ptr = next.ptr;
#if defined(__ppc__) || defined(__powerpc__) || defined(__arm__)
        if (cas((long*)&pQueue->head, atomic_ptr_to_long(head), atomic_ptr_to_long(node)) == atomic_ptr_to_long(head))
#else
        node.version = head.version + 1;
        if (cas2((long long*)&pQueue->head, atomic_ptr_to_long_long(head), atomic_ptr_to_long_long(node)) == atomic_ptr_to_long_long(head))
#endif
          break; // Dequeue is done
      }
    }
  } while (true); // Keep trying until the dequeue is done or the queue empties
  lf_queue_free_node(pQueue, head.ptr);
  AtomicDecrement(&pQueue->len);
  return pVal;
}

#ifdef __ppc__
#pragma GCC optimization_level reset
#endif
