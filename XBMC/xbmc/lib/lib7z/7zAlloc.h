/* 7zAlloc.h */

#ifndef __7Z_ALLOC_H
#define __7Z_ALLOC_H

#include <stddef.h>

typedef struct _ISzAlloc
{
  void *(*Alloc)(size_t size);
  void (*Free)(void *address); /* address can be 0 */
} ISzAlloc;

void *SzAlloc(size_t size);
void SzFree(void *address);

void *SzAllocTemp(size_t size);
void SzFreeTemp(void *address);

#endif
