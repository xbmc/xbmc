/* 7zExtract.c */
#include <stdio.h>
#include "7zExtract.h"
#include "7zDecode.h"
#include "7zCrc.h"

SZ_RESULT SzExtract(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    UInt32 fileIndex,
    UInt32 *blockIndex,
    Byte **outBuffer, 
    size_t *outBufferSize,
    size_t *offset, 
    size_t *outSizeProcessed, 
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp,
    size_t *nowPos,
    WriteCache *writeCache,
    void *writeOBJECT,
    int SizeToCache,
    ReadCache *readCache,
    void *readOBJECT)
{
  printf("SzExtract\n");
  UInt32 folderIndex = db->FileIndexToFolderIndexMap[fileIndex];
  SZ_RESULT res = SZ_OK;
  *offset = 0;
  *outSizeProcessed = 0;
  if (folderIndex == (UInt32)-1)
  {
    allocMain->Free(*outBuffer);
    *blockIndex = folderIndex;
    if (readCache == NULL || readOBJECT == NULL)
      *outBuffer = 0;
    *outBufferSize = 0;
    return SZ_OK;
  }

  if (/**outBuffer == 0 ||*/ *blockIndex != folderIndex)
  {
    CFolder *folder = db->Database.Folders + folderIndex;
    CFileSize unPackSizeSpec = SzFolderGetUnPackSize(folder);
    size_t unPackSize = (size_t)unPackSizeSpec;
    CFileSize startOffset = SzArDbGetFolderStreamPos(db, folderIndex, 0);
    #ifndef _LZMA_IN_CB
    Byte *inBuffer = 0;
    size_t processedSize;
    CFileSize packSizeSpec;
    size_t packSize;
    RINOK(SzArDbGetFolderFullPackSize(db, folderIndex, &packSizeSpec));
    packSize = (size_t)packSizeSpec;
    if (packSize != packSizeSpec)
      return SZE_OUTOFMEMORY;
    #endif
    if (unPackSize != unPackSizeSpec)
      return SZE_OUTOFMEMORY;
    *blockIndex = folderIndex;
    if (readCache == NULL || readOBJECT == NULL)
    {
      allocMain->Free(*outBuffer);
      *outBuffer = 0;
    }
    
    RINOK(inStream->Seek(inStream, startOffset));
    
    #ifndef _LZMA_IN_CB
    if (packSize != 0)
    {
      inBuffer = (Byte *)allocTemp->Alloc(packSize);
      if (inBuffer == 0)
        return SZE_OUTOFMEMORY;
    }
    res = inStream->Read(inStream, inBuffer, packSize, &processedSize);
    if (res == SZ_OK && processedSize != packSize)
      res = SZE_FAIL;
    #endif
    if (res == SZ_OK)
    {
      *outBufferSize = unPackSize;
      if (unPackSize != 0)
      {
        if (readCache == NULL || readOBJECT == NULL)
        {
          *outBuffer = (Byte *)allocMain->Alloc(unPackSize);
          if (*outBuffer == 0)
            res = SZE_OUTOFMEMORY;
        }
      }
      if (res == SZ_OK)
      {
        res = SzDecode(db->Database.PackSizes + 
          db->FolderStartPackStreamIndex[folderIndex], folder, 
          #ifdef _LZMA_IN_CB
          inStream, startOffset, 
          #else
          inBuffer, 
          #endif
          *outBuffer, unPackSize, allocTemp, nowPos, writeCache, writeOBJECT, SizeToCache, readCache, readOBJECT);
        if (res == SZ_OK)
        {
          if (folder->UnPackCRCDefined)
          {
            if (readCache == NULL || readOBJECT == NULL)
            {
              if (CrcCalc(*outBuffer, unPackSize) != folder->UnPackCRC)
                res = SZE_CRC_ERROR;
            }
          }
        }
      }
    }
    #ifndef _LZMA_IN_CB
    allocTemp->Free(inBuffer);
    #endif
  }
  if (res == SZ_OK)
  {
    UInt32 i; 
    C7zFileItem *fileItem = db->Database.Files + fileIndex;
    *offset = 0;
    for(i = db->FolderStartFileIndex[folderIndex]; i < fileIndex; i++)
      *offset += (UInt32)db->Database.Files[i].Size;
    *outSizeProcessed = (size_t)fileItem->Size;
    if (*offset + *outSizeProcessed > *outBufferSize)
      return SZE_FAIL;
    {
      if (fileItem->IsFileCRCDefined)
      {
        if (readCache == NULL || readOBJECT == NULL)
        {
          if (CrcCalc(*outBuffer + *offset, *outSizeProcessed) != fileItem->FileCRC)
            res = SZE_CRC_ERROR;
        }
      }
    }
  }
  return res;
}
