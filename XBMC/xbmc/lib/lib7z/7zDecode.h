/* 7zDecode.h */

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "7zItem.h"
#include "7zAlloc.h"
#ifdef _LZMA_IN_CB
#include "7zIn.h"
#endif
typedef bool (WriteCache)(void *object, unsigned char *buffer, int size);
typedef Byte (ReadCache)(void *object, size_t position);
SZ_RESULT SzDecode(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *stream, CFileSize startPos,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain, size_t *nowPos,     WriteCache *writeCache,
    void *writeOBJECT, int SizeToCache, ReadCache *readCache, void *readOBJECT);

#endif
