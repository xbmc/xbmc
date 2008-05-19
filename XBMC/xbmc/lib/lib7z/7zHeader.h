/* 7zHeader.h */

#ifndef __7Z_HEADER_H
#define __7Z_HEADER_H

#include "Types.h"

#define k7zSignatureSize 6
extern Byte k7zSignature[k7zSignatureSize];

#define k7zMajorVersion 0

#define k7zStartHeaderSize 0x20

enum EIdEnum
{
  k7zIdEnd,
    
  k7zIdHeader,
    
  k7zIdArchiveProperties,
    
  k7zIdAdditionalStreamsInfo,
  k7zIdMainStreamsInfo,
  k7zIdFilesInfo,
  
  k7zIdPackInfo,
  k7zIdUnPackInfo,
  k7zIdSubStreamsInfo,
  
  k7zIdSize,
  k7zIdCRC,
  
  k7zIdFolder,
  
  k7zIdCodersUnPackSize,
  k7zIdNumUnPackStream,
  
  k7zIdEmptyStream,
  k7zIdEmptyFile,
  k7zIdAnti,
  
  k7zIdName,
  k7zIdCreationTime,
  k7zIdLastAccessTime,
  k7zIdLastWriteTime,
  k7zIdWinAttributes,
  k7zIdComment,
  
  k7zIdEncodedHeader,
  
  k7zIdStartPos
};

#endif
