/* 7zBuffer.h */

#ifndef __7Z_BUFFER_H
#define __7Z_BUFFER_H

#include <stddef.h>
#include "Types.h"

typedef struct _CSzByteBuffer
{
  size_t Capacity;
  Byte *Items;
}CSzByteBuffer;

void SzByteBufferInit(CSzByteBuffer *buffer);
int SzByteBufferCreate(CSzByteBuffer *buffer, size_t newCapacity, void * (*allocFunc)(size_t size));
void SzByteBufferFree(CSzByteBuffer *buffer, void (*freeFunc)(void *));

#endif
