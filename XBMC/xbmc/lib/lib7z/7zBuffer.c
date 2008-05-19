/* 7zBuffer.c */

#include "7zBuffer.h"
#include "7zAlloc.h"

void SzByteBufferInit(CSzByteBuffer *buffer)
{
  buffer->Capacity = 0;
  buffer->Items = 0;
}

int SzByteBufferCreate(CSzByteBuffer *buffer, size_t newCapacity, void * (*allocFunc)(size_t size))
{
  buffer->Capacity = newCapacity;
  if (newCapacity == 0)
  {
    buffer->Items = 0;
    return 1;
  }
  buffer->Items = (Byte *)allocFunc(newCapacity);
  return (buffer->Items != 0);
}

void SzByteBufferFree(CSzByteBuffer *buffer, void (*freeFunc)(void *))
{
  freeFunc(buffer->Items);
  buffer->Items = 0;
  buffer->Capacity = 0;
}
