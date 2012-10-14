#include "gridfs.h"
#include "prepostChunkProcessing.h"
#include "zlib.h"

static int ZlibPreProcessChunk(void** targetBuf, size_t* targetLen, void* srcBuf, size_t srcLen, int flags) {
  uLongf tmpLen = compressBound( DEFAULT_CHUNK_SIZE );
  if( flags & GRIDFILE_COMPRESS ) {    
    if( *targetBuf == NULL ) {
      *targetBuf = bson_malloc( tmpLen );
    }
    if( compress2( (Bytef*)(*targetBuf), &tmpLen, (Bytef*)srcBuf, (uLong)srcLen, Z_BEST_SPEED ) != Z_OK ) {
      return -1;
    }    
    *targetLen = (size_t)tmpLen;
  } else {
    *targetBuf = (void*)srcBuf;
    *targetLen = srcLen;
  }
  return 0;
}

static int ZlibPostProcessChunk(void** targetBuf, size_t* targetLen, void* srcData, size_t srcLen, int flags) {   
  uLongf tmpLen = DEFAULT_CHUNK_SIZE;
  if( flags & GRIDFILE_COMPRESS ) {  
    if( *targetBuf == NULL ) {
      *targetBuf = (void*)bson_malloc( tmpLen );
    }  
    if (uncompress( (Bytef*)(*targetBuf), &tmpLen, (Bytef*)srcData, (uLong)srcLen ) != Z_OK ) {
      return -1;
    }
    *targetLen = (size_t)tmpLen;
  } else {
    *targetBuf = srcData;
    *targetLen = srcLen;
  }
  return 0;
}

static size_t ZlibPendingDataNeededSize (int flags) {
  if( flags & GRIDFILE_COMPRESS ) {
    return compressBound( DEFAULT_CHUNK_SIZE );
  } else {
    return DEFAULT_CHUNK_SIZE;
  }
}

MONGO_EXPORT int initPrepostChunkProcessing( int flags ){
  setBufferProcessingProcs( ZlibPreProcessChunk, ZlibPostProcessChunk, ZlibPendingDataNeededSize );  
  return 0;
}

