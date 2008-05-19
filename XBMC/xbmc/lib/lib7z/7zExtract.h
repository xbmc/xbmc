/* 7zExtract.h */

#ifndef __7Z_EXTRACT_H
#define __7Z_EXTRACT_H

#include "7zIn.h"
typedef bool (WriteCache)(void *object, unsigned char *buffer, int size);
typedef Byte (ReadCache)(void *object, size_t position);
/*
  SzExtract extracts file from archive

  *outBuffer must be 0 before first call for each new archive. 

  Extracting cache:
    If you need to decompress more than one file, you can send 
    these values from previous call:
      *blockIndex, 
      *outBuffer, 
      *outBufferSize
    You can consider "*outBuffer" as cache of solid block. If your archive is solid, 
    it will increase decompression speed.
  
    If you use external function, you can declare these 3 cache variables 
    (blockIndex, outBuffer, outBufferSize) as static in that external function.
    
    Free *outBuffer and set *outBuffer to 0, if you want to flush cache.
*/

SZ_RESULT SzExtract(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    UInt32 fileIndex,         /* index of file */
    UInt32 *blockIndex,       /* index of solid block */
    Byte **outBuffer,         /* pointer to pointer to output buffer (allocated with allocMain) */
    size_t *outBufferSize,    /* buffer size for output buffer */
    size_t *offset,           /* offset of stream for required file in *outBuffer */
    size_t *outSizeProcessed, /* size of file in *outBuffer */
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp,
    size_t *nowPos,
    WriteCache *writeCache,
    void *writeOBJECT,
    int SizeToCache,
    ReadCache *readCache,
    void *readOBJECT);

#endif
