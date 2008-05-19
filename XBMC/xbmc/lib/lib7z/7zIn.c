/* 7zIn.c */

#include "7zIn.h"
#include "7zDecode.h"
#include "7zCrc.h"

#define RINOM(x) { if((x) == 0) return SZE_OUTOFMEMORY; }

void SzArDbExInit(CArchiveDatabaseEx *db)
{
  SzArchiveDatabaseInit(&db->Database);
  db->FolderStartPackStreamIndex = 0;
  db->PackStreamStartPositions = 0;
  db->FolderStartFileIndex = 0;
  db->FileIndexToFolderIndexMap = 0;
}

void SzArDbExFree(CArchiveDatabaseEx *db, void (*freeFunc)(void *))
{
  freeFunc(db->FolderStartPackStreamIndex);
  freeFunc(db->PackStreamStartPositions);
  freeFunc(db->FolderStartFileIndex);
  freeFunc(db->FileIndexToFolderIndexMap);
  SzArchiveDatabaseFree(&db->Database, freeFunc);
  SzArDbExInit(db);
}

/*
CFileSize GetFolderPackStreamSize(int folderIndex, int streamIndex) const 
{
  return PackSizes[FolderStartPackStreamIndex[folderIndex] + streamIndex];
}

CFileSize GetFilePackSize(int fileIndex) const
{
  int folderIndex = FileIndexToFolderIndexMap[fileIndex];
  if (folderIndex >= 0)
  {
    const CFolder &folderInfo = Folders[folderIndex];
    if (FolderStartFileIndex[folderIndex] == fileIndex)
    return GetFolderFullPackSize(folderIndex);
  }
  return 0;
}
*/

#define MY_ALLOC(T, p, size, allocFunc) { if ((size) == 0) p = 0; else \
  if ((p = (T *)allocFunc((size) * sizeof(T))) == 0) return SZE_OUTOFMEMORY; }

SZ_RESULT SzArDbExFill(CArchiveDatabaseEx *db, void * (*allocFunc)(size_t size))
{
  UInt32 startPos = 0;
  CFileSize startPosSize = 0;
  UInt32 i;
  UInt32 folderIndex = 0;
  UInt32 indexInFolder = 0;
  MY_ALLOC(UInt32, db->FolderStartPackStreamIndex, db->Database.NumFolders, allocFunc);
  for(i = 0; i < db->Database.NumFolders; i++)
  {
    db->FolderStartPackStreamIndex[i] = startPos;
    startPos += db->Database.Folders[i].NumPackStreams;
  }

  MY_ALLOC(CFileSize, db->PackStreamStartPositions, db->Database.NumPackStreams, allocFunc);

  for(i = 0; i < db->Database.NumPackStreams; i++)
  {
    db->PackStreamStartPositions[i] = startPosSize;
    startPosSize += db->Database.PackSizes[i];
  }

  MY_ALLOC(UInt32, db->FolderStartFileIndex, db->Database.NumFolders, allocFunc);
  MY_ALLOC(UInt32, db->FileIndexToFolderIndexMap, db->Database.NumFiles, allocFunc);

  for (i = 0; i < db->Database.NumFiles; i++)
  {
    C7zFileItem *file = db->Database.Files + i;
    int emptyStream = !file->HasStream;
    if (emptyStream && indexInFolder == 0)
    {
      db->FileIndexToFolderIndexMap[i] = (UInt32)-1;
      continue;
    }
    if (indexInFolder == 0)
    {
      /*
      v3.13 incorrectly worked with empty folders
      v4.07: Loop for skipping empty folders
      */
      for (;;)
      {
        if (folderIndex >= db->Database.NumFolders)
          return SZE_ARCHIVE_ERROR;
        db->FolderStartFileIndex[folderIndex] = i;
        if (db->Database.Folders[folderIndex].NumUnPackStreams != 0)
          break;
        folderIndex++;
      }
    }
    db->FileIndexToFolderIndexMap[i] = folderIndex;
    if (emptyStream)
      continue;
    indexInFolder++;
    if (indexInFolder >= db->Database.Folders[folderIndex].NumUnPackStreams)
    {
      folderIndex++;
      indexInFolder = 0;
    }
  }
  return SZ_OK;
}


CFileSize SzArDbGetFolderStreamPos(CArchiveDatabaseEx *db, UInt32 folderIndex, UInt32 indexInFolder)
{
  return db->ArchiveInfo.DataStartPosition + 
    db->PackStreamStartPositions[db->FolderStartPackStreamIndex[folderIndex] + indexInFolder];
}

int SzArDbGetFolderFullPackSize(CArchiveDatabaseEx *db, UInt32 folderIndex, CFileSize *resSize)
{
  UInt32 packStreamIndex = db->FolderStartPackStreamIndex[folderIndex];
  CFolder *folder = db->Database.Folders + folderIndex;
  CFileSize size = 0;
  UInt32 i;
  for (i = 0; i < folder->NumPackStreams; i++)
  {
    CFileSize t = size + db->Database.PackSizes[packStreamIndex + i];
    if (t < size)
      return SZE_FAIL;
    size = t;
  }
  *resSize = size;
  return SZ_OK;
}


/*
SZ_RESULT SzReadTime(const CObjectVector<CSzByteBuffer> &dataVector,
    CObjectVector<CFileItem> &files, UInt64 type)
{
  CBoolVector boolVector;
  RINOK(ReadBoolVector2(files.Size(), boolVector))

  CStreamSwitch streamSwitch;
  RINOK(streamSwitch.Set(this, &dataVector));

  for(int i = 0; i < files.Size(); i++)
  {
    CFileItem &file = files[i];
    CArchiveFileTime fileTime;
    bool defined = boolVector[i];
    if (defined)
    {
      UInt32 low, high;
      RINOK(SzReadUInt32(low));
      RINOK(SzReadUInt32(high));
      fileTime.dwLowDateTime = low;
      fileTime.dwHighDateTime = high;
    }
    switch(type)
    {
      case k7zIdCreationTime:
        file.IsCreationTimeDefined = defined;
        if (defined)
          file.CreationTime = fileTime;
        break;
      case k7zIdLastWriteTime:
        file.IsLastWriteTimeDefined = defined;
        if (defined)
          file.LastWriteTime = fileTime;
        break;
      case k7zIdLastAccessTime:
        file.IsLastAccessTimeDefined = defined;
        if (defined)
          file.LastAccessTime = fileTime;
        break;
    }
  }
  return SZ_OK;
}
*/

SZ_RESULT SafeReadDirect(ISzInStream *inStream, Byte *data, size_t size)
{
  #ifdef _LZMA_IN_CB
  while (size > 0)
  {
    void *inBufferSpec;
    size_t processedSize;
    const Byte *inBuffer;
    RINOK(inStream->Read(inStream, (void **)&inBufferSpec, size, &processedSize));
    inBuffer = (const Byte *)inBufferSpec;
    if (processedSize == 0 || processedSize > size)
      return SZE_FAIL;
    size -= processedSize;
    do
    {
      *data++ = *inBuffer++;
    }
    while (--processedSize != 0);
  }
  #else
  size_t processedSize;
  RINOK(inStream->Read(inStream, data, size, &processedSize));
  if (processedSize != size)
    return SZE_FAIL;
  #endif
  return SZ_OK;
}

SZ_RESULT SafeReadDirectByte(ISzInStream *inStream, Byte *data)
{
  return SafeReadDirect(inStream, data, 1);
}

SZ_RESULT SafeReadDirectUInt32(ISzInStream *inStream, UInt32 *value, UInt32 *crc)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt32)b << (8 * i));
    *crc = CRC_UPDATE_BYTE(*crc, b);
  }
  return SZ_OK;
}

SZ_RESULT SafeReadDirectUInt64(ISzInStream *inStream, UInt64 *value, UInt32 *crc)
{
  int i;
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt64)b << (8 * i));
    *crc = CRC_UPDATE_BYTE(*crc, b);
  }
  return SZ_OK;
}

int TestSignatureCandidate(Byte *testBytes)
{
  size_t i;
  for (i = 0; i < k7zSignatureSize; i++)
    if (testBytes[i] != k7zSignature[i])
      return 0;
  return 1;
}

typedef struct _CSzState
{
  Byte *Data;
  size_t Size;
}CSzData;

SZ_RESULT SzReadByte(CSzData *sd, Byte *b)
{
  if (sd->Size == 0)
    return SZE_ARCHIVE_ERROR;
  sd->Size--;
  *b = *sd->Data++;
  return SZ_OK;
}

SZ_RESULT SzReadBytes(CSzData *sd, Byte *data, size_t size)
{
  size_t i;
  for (i = 0; i < size; i++)
  {
    RINOK(SzReadByte(sd, data + i));
  }
  return SZ_OK;
}

SZ_RESULT SzReadUInt32(CSzData *sd, UInt32 *value)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt32)(b) << (8 * i));
  }
  return SZ_OK;
}

SZ_RESULT SzReadNumber(CSzData *sd, UInt64 *value)
{
  Byte firstByte;
  Byte mask = 0x80;
  int i;
  RINOK(SzReadByte(sd, &firstByte));
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    if ((firstByte & mask) == 0)
    {
      UInt64 highPart = firstByte & (mask - 1);
      *value += (highPart << (8 * i));
      return SZ_OK;
    }
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt64)b << (8 * i));
    mask >>= 1;
  }
  return SZ_OK;
}

SZ_RESULT SzReadSize(CSzData *sd, CFileSize *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  *value = (CFileSize)value64;
  return SZ_OK;
}

SZ_RESULT SzReadNumber32(CSzData *sd, UInt32 *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  if (value64 >= 0x80000000)
    return SZE_NOTIMPL;
  if (value64 >= ((UInt64)(1) << ((sizeof(size_t) - 1) * 8 + 2)))
    return SZE_NOTIMPL;
  *value = (UInt32)value64;
  return SZ_OK;
}

SZ_RESULT SzReadID(CSzData *sd, UInt64 *value) 
{ 
  return SzReadNumber(sd, value); 
}

SZ_RESULT SzSkeepDataSize(CSzData *sd, UInt64 size)
{
  if (size > sd->Size)
    return SZE_ARCHIVE_ERROR;
  sd->Size -= (size_t)size;
  sd->Data += (size_t)size;
  return SZ_OK;
}

SZ_RESULT SzSkeepData(CSzData *sd)
{
  UInt64 size;
  RINOK(SzReadNumber(sd, &size));
  return SzSkeepDataSize(sd, size);
}

SZ_RESULT SzReadArchiveProperties(CSzData *sd)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    SzSkeepData(sd);
  }
  return SZ_OK;
}

SZ_RESULT SzWaitAttribute(CSzData *sd, UInt64 attribute)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == attribute)
      return SZ_OK;
    if (type == k7zIdEnd)
      return SZE_ARCHIVE_ERROR;
    RINOK(SzSkeepData(sd));
  }
}

SZ_RESULT SzReadBoolVector(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size))
{
  Byte b = 0;
  Byte mask = 0;
  size_t i;
  MY_ALLOC(Byte, *v, numItems, allocFunc);
  for (i = 0; i < numItems; i++)
  {
    if (mask == 0)
    {
      RINOK(SzReadByte(sd, &b));
      mask = 0x80;
    }
    (*v)[i] = (Byte)(((b & mask) != 0) ? 1 : 0);
    mask >>= 1;
  }
  return SZ_OK;
}

SZ_RESULT SzReadBoolVector2(CSzData *sd, size_t numItems, Byte **v, void * (*allocFunc)(size_t size))
{
  Byte allAreDefined;
  size_t i;
  RINOK(SzReadByte(sd, &allAreDefined));
  if (allAreDefined == 0)
    return SzReadBoolVector(sd, numItems, v, allocFunc);
  MY_ALLOC(Byte, *v, numItems, allocFunc);
  for(i = 0; i < numItems; i++)
    (*v)[i] = 1;
  return SZ_OK;
}

SZ_RESULT SzReadHashDigests(
    CSzData *sd, 
    size_t numItems,
    Byte **digestsDefined, 
    UInt32 **digests, 
    void * (*allocFunc)(size_t size))
{
  size_t i;
  RINOK(SzReadBoolVector2(sd, numItems, digestsDefined, allocFunc));
  MY_ALLOC(UInt32, *digests, numItems, allocFunc);
  for(i = 0; i < numItems; i++)
    if ((*digestsDefined)[i])
    {
      RINOK(SzReadUInt32(sd, (*digests) + i));
    }
  return SZ_OK;
}

SZ_RESULT SzReadPackInfo(
    CSzData *sd, 
    CFileSize *dataOffset,
    UInt32 *numPackStreams,
    CFileSize **packSizes,
    Byte **packCRCsDefined,
    UInt32 **packCRCs,
    void * (*allocFunc)(size_t size))
{
  UInt32 i;
  RINOK(SzReadSize(sd, dataOffset));
  RINOK(SzReadNumber32(sd, numPackStreams));

  RINOK(SzWaitAttribute(sd, k7zIdSize));

  MY_ALLOC(CFileSize, *packSizes, (size_t)*numPackStreams, allocFunc);

  for(i = 0; i < *numPackStreams; i++)
  {
    RINOK(SzReadSize(sd, (*packSizes) + i));
  }

  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    if (type == k7zIdCRC)
    {
      RINOK(SzReadHashDigests(sd, (size_t)*numPackStreams, packCRCsDefined, packCRCs, allocFunc)); 
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
  if (*packCRCsDefined == 0)
  {
    MY_ALLOC(Byte, *packCRCsDefined, (size_t)*numPackStreams, allocFunc);
    MY_ALLOC(UInt32, *packCRCs, (size_t)*numPackStreams, allocFunc);
    for(i = 0; i < *numPackStreams; i++)
    {
      (*packCRCsDefined)[i] = 0;
      (*packCRCs)[i] = 0;
    }
  }
  return SZ_OK;
}

SZ_RESULT SzReadSwitch(CSzData *sd)
{
  Byte external;
  RINOK(SzReadByte(sd, &external));
  return (external == 0) ? SZ_OK: SZE_ARCHIVE_ERROR;
}

SZ_RESULT SzGetNextFolderItem(CSzData *sd, CFolder *folder, void * (*allocFunc)(size_t size))
{
  UInt32 numCoders;
  UInt32 numBindPairs;
  UInt32 numPackedStreams;
  UInt32 i;
  UInt32 numInStreams = 0;
  UInt32 numOutStreams = 0;
  RINOK(SzReadNumber32(sd, &numCoders));
  folder->NumCoders = numCoders;

  MY_ALLOC(CCoderInfo, folder->Coders, (size_t)numCoders, allocFunc);

  for (i = 0; i < numCoders; i++)
    SzCoderInfoInit(folder->Coders + i);

  for (i = 0; i < numCoders; i++)
  {
    Byte mainByte;
    CCoderInfo *coder = folder->Coders + i;
    {
      unsigned idSize, j;
      Byte longID[15];
      RINOK(SzReadByte(sd, &mainByte));
      idSize = (unsigned)(mainByte & 0xF);
      RINOK(SzReadBytes(sd, longID, idSize));
      if (idSize > sizeof(coder->MethodID))
        return SZE_NOTIMPL;
      coder->MethodID = 0;
      for (j = 0; j < idSize; j++)
        coder->MethodID |= (CMethodID)longID[idSize - 1 - j] << (8 * j);

      if ((mainByte & 0x10) != 0)
      {
        RINOK(SzReadNumber32(sd, &coder->NumInStreams));
        RINOK(SzReadNumber32(sd, &coder->NumOutStreams));
      }
      else
      {
        coder->NumInStreams = 1;
        coder->NumOutStreams = 1;
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        if (!SzByteBufferCreate(&coder->Properties, (size_t)propertiesSize, allocFunc))
          return SZE_OUTOFMEMORY;
        RINOK(SzReadBytes(sd, coder->Properties.Items, (size_t)propertiesSize));
      }
    }
    while ((mainByte & 0x80) != 0)
    {
      RINOK(SzReadByte(sd, &mainByte));
      RINOK(SzSkeepDataSize(sd, (mainByte & 0xF)));
      if ((mainByte & 0x10) != 0)
      {
        UInt32 n;
        RINOK(SzReadNumber32(sd, &n));
        RINOK(SzReadNumber32(sd, &n));
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        RINOK(SzSkeepDataSize(sd, propertiesSize));
      }
    }
    numInStreams += (UInt32)coder->NumInStreams;
    numOutStreams += (UInt32)coder->NumOutStreams;
  }

  numBindPairs = numOutStreams - 1;
  folder->NumBindPairs = numBindPairs;


  MY_ALLOC(CBindPair, folder->BindPairs, (size_t)numBindPairs, allocFunc);

  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair *bindPair = folder->BindPairs + i;;
    RINOK(SzReadNumber32(sd, &bindPair->InIndex));
    RINOK(SzReadNumber32(sd, &bindPair->OutIndex)); 
  }

  numPackedStreams = numInStreams - (UInt32)numBindPairs;

  folder->NumPackStreams = numPackedStreams;
  MY_ALLOC(UInt32, folder->PackStreams, (size_t)numPackedStreams, allocFunc);

  if (numPackedStreams == 1)
  {
    UInt32 j;
    UInt32 pi = 0;
    for (j = 0; j < numInStreams; j++)
      if (SzFolderFindBindPairForInStream(folder, j) < 0)
      {
        folder->PackStreams[pi++] = j;
        break;
      }
  }
  else
    for(i = 0; i < numPackedStreams; i++)
    {
      RINOK(SzReadNumber32(sd, folder->PackStreams + i));
    }
  return SZ_OK;
}

SZ_RESULT SzReadUnPackInfo(
    CSzData *sd, 
    UInt32 *numFolders,
    CFolder **folders,  /* for allocFunc */
    void * (*allocFunc)(size_t size),
    ISzAlloc *allocTemp)
{
  UInt32 i;
  RINOK(SzWaitAttribute(sd, k7zIdFolder));
  RINOK(SzReadNumber32(sd, numFolders));
  {
    RINOK(SzReadSwitch(sd));

    MY_ALLOC(CFolder, *folders, (size_t)*numFolders, allocFunc);

    for(i = 0; i < *numFolders; i++)
      SzFolderInit((*folders) + i);

    for(i = 0; i < *numFolders; i++)
    {
      RINOK(SzGetNextFolderItem(sd, (*folders) + i, allocFunc));
    }
  }

  RINOK(SzWaitAttribute(sd, k7zIdCodersUnPackSize));

  for(i = 0; i < *numFolders; i++)
  {
    UInt32 j;
    CFolder *folder = (*folders) + i;
    UInt32 numOutStreams = SzFolderGetNumOutStreams(folder);

    MY_ALLOC(CFileSize, folder->UnPackSizes, (size_t)numOutStreams, allocFunc);

    for(j = 0; j < numOutStreams; j++)
    {
      RINOK(SzReadSize(sd, folder->UnPackSizes + j));
    }
  }

  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      return SZ_OK;
    if (type == k7zIdCRC)
    {
      SZ_RESULT res;
      Byte *crcsDefined = 0;
      UInt32 *crcs = 0;
      res = SzReadHashDigests(sd, *numFolders, &crcsDefined, &crcs, allocTemp->Alloc); 
      if (res == SZ_OK)
      {
        for(i = 0; i < *numFolders; i++)
        {
          CFolder *folder = (*folders) + i;
          folder->UnPackCRCDefined = crcsDefined[i];
          folder->UnPackCRC = crcs[i];
        }
      }
      allocTemp->Free(crcs);
      allocTemp->Free(crcsDefined);
      RINOK(res);
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
}

SZ_RESULT SzReadSubStreamsInfo(
    CSzData *sd, 
    UInt32 numFolders,
    CFolder *folders,
    UInt32 *numUnPackStreams,
    CFileSize **unPackSizes,
    Byte **digestsDefined,
    UInt32 **digests,
    ISzAlloc *allocTemp)
{
  UInt64 type = 0;
  UInt32 i;
  UInt32 si = 0;
  UInt32 numDigests = 0;

  for(i = 0; i < numFolders; i++)
    folders[i].NumUnPackStreams = 1;
  *numUnPackStreams = numFolders;

  for (;;)
  {
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdNumUnPackStream)
    {
      *numUnPackStreams = 0;
      for(i = 0; i < numFolders; i++)
      {
        UInt32 numStreams;
        RINOK(SzReadNumber32(sd, &numStreams));
        folders[i].NumUnPackStreams = numStreams;
        *numUnPackStreams += numStreams;
      }
      continue;
    }
    if (type == k7zIdCRC || type == k7zIdSize)
      break;
    if (type == k7zIdEnd)
      break;
    RINOK(SzSkeepData(sd));
  }

  if (*numUnPackStreams == 0)
  {
    *unPackSizes = 0;
    *digestsDefined = 0;
    *digests = 0;
  }
  else
  {
    *unPackSizes = (CFileSize *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(CFileSize));
    RINOM(*unPackSizes);
    *digestsDefined = (Byte *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(Byte));
    RINOM(*digestsDefined);
    *digests = (UInt32 *)allocTemp->Alloc((size_t)*numUnPackStreams * sizeof(UInt32));
    RINOM(*digests);
  }

  for(i = 0; i < numFolders; i++)
  {
    /*
    v3.13 incorrectly worked with empty folders
    v4.07: we check that folder is empty
    */
    CFileSize sum = 0;
    UInt32 j;
    UInt32 numSubstreams = folders[i].NumUnPackStreams;
    if (numSubstreams == 0)
      continue;
    if (type == k7zIdSize)
    for (j = 1; j < numSubstreams; j++)
    {
      CFileSize size;
      RINOK(SzReadSize(sd, &size));
      (*unPackSizes)[si++] = size;
      sum += size;
    }
    (*unPackSizes)[si++] = SzFolderGetUnPackSize(folders + i) - sum;
  }
  if (type == k7zIdSize)
  {
    RINOK(SzReadID(sd, &type));
  }

  for(i = 0; i < *numUnPackStreams; i++)
  {
    (*digestsDefined)[i] = 0;
    (*digests)[i] = 0;
  }


  for(i = 0; i < numFolders; i++)
  {
    UInt32 numSubstreams = folders[i].NumUnPackStreams;
    if (numSubstreams != 1 || !folders[i].UnPackCRCDefined)
      numDigests += numSubstreams;
  }

 
  si = 0;
  for (;;)
  {
    if (type == k7zIdCRC)
    {
      int digestIndex = 0;
      Byte *digestsDefined2 = 0; 
      UInt32 *digests2 = 0;
      SZ_RESULT res = SzReadHashDigests(sd, numDigests, &digestsDefined2, &digests2, allocTemp->Alloc);
      if (res == SZ_OK)
      {
        for (i = 0; i < numFolders; i++)
        {
          CFolder *folder = folders + i;
          UInt32 numSubstreams = folder->NumUnPackStreams;
          if (numSubstreams == 1 && folder->UnPackCRCDefined)
          {
            (*digestsDefined)[si] = 1;
            (*digests)[si] = folder->UnPackCRC;
            si++;
          }
          else
          {
            UInt32 j;
            for (j = 0; j < numSubstreams; j++, digestIndex++)
            {
              (*digestsDefined)[si] = digestsDefined2[digestIndex];
              (*digests)[si] = digests2[digestIndex];
              si++;
            }
          }
        }
      }
      allocTemp->Free(digestsDefined2);
      allocTemp->Free(digests2);
      RINOK(res);
    }
    else if (type == k7zIdEnd)
      return SZ_OK;
    else
    {
      RINOK(SzSkeepData(sd));
    }
    RINOK(SzReadID(sd, &type));
  }
}


SZ_RESULT SzReadStreamsInfo(
    CSzData *sd, 
    CFileSize *dataOffset,
    CArchiveDatabase *db,
    UInt32 *numUnPackStreams,
    CFileSize **unPackSizes, /* allocTemp */
    Byte **digestsDefined,   /* allocTemp */
    UInt32 **digests,        /* allocTemp */
    void * (*allocFunc)(size_t size),
    ISzAlloc *allocTemp)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if ((UInt64)(int)type != type)
      return SZE_FAIL;
    switch((int)type)
    {
      case k7zIdEnd:
        return SZ_OK;
      case k7zIdPackInfo:
      {
        RINOK(SzReadPackInfo(sd, dataOffset, &db->NumPackStreams, 
            &db->PackSizes, &db->PackCRCsDefined, &db->PackCRCs, allocFunc));
        break;
      }
      case k7zIdUnPackInfo:
      {
        RINOK(SzReadUnPackInfo(sd, &db->NumFolders, &db->Folders, allocFunc, allocTemp));
        break;
      }
      case k7zIdSubStreamsInfo:
      {
        RINOK(SzReadSubStreamsInfo(sd, db->NumFolders, db->Folders, 
            numUnPackStreams, unPackSizes, digestsDefined, digests, allocTemp));
        break;
      }
      default:
        return SZE_FAIL;
    }
  }
}

Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

SZ_RESULT SzReadFileNames(CSzData *sd, UInt32 numFiles, C7zFileItem *files, 
    void * (*allocFunc)(size_t size))
{
  UInt32 i;
  for(i = 0; i < numFiles; i++)
  {
    UInt32 len = 0;
    UInt32 pos = 0;
    C7zFileItem *file = files + i;
    while(pos + 2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
      pos += 2;
      len++;
      if (value == 0)
        break;
      if (value < 0x80)
        continue;
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2;
        if (value >= 0xDC00)
          return SZE_ARCHIVE_ERROR;
        if (pos + 2 > sd->Size)
          return SZE_ARCHIVE_ERROR;
        c2 = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
        pos += 2;
        if (c2 < 0xDC00 || c2 >= 0xE000)
          return SZE_ARCHIVE_ERROR;
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      len += numAdds;
    }

    MY_ALLOC(char, file->Name, (size_t)len, allocFunc);

    len = 0;
    while(2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
      SzSkeepDataSize(sd, 2);
      if (value < 0x80)
      {
        file->Name[len++] = (char)value;
        if (value == 0)
          break;
        continue;
      }
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2 = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
        SzSkeepDataSize(sd, 2);
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      file->Name[len++] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
      do
      {
        numAdds--;
        file->Name[len++] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
      }
      while(numAdds > 0);

      len += numAdds;
    }
  }
  return SZ_OK;
}

SZ_RESULT SzReadHeader2(
    CSzData *sd, 
    CArchiveDatabaseEx *db,   /* allocMain */
    CFileSize **unPackSizes,  /* allocTemp */
    Byte **digestsDefined,    /* allocTemp */
    UInt32 **digests,         /* allocTemp */
    Byte **emptyStreamVector, /* allocTemp */
    Byte **emptyFileVector,   /* allocTemp */
    Byte **lwtVector,         /* allocTemp */
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  UInt64 type;
  UInt32 numUnPackStreams = 0;
  UInt32 numFiles = 0;
  C7zFileItem *files = 0;
  UInt32 numEmptyStreams = 0;
  UInt32 i;

  RINOK(SzReadID(sd, &type));

  if (type == k7zIdArchiveProperties)
  {
    RINOK(SzReadArchiveProperties(sd));
    RINOK(SzReadID(sd, &type));
  }
 
 
  if (type == k7zIdMainStreamsInfo)
  {
    RINOK(SzReadStreamsInfo(sd,
        &db->ArchiveInfo.DataStartPosition,
        &db->Database, 
        &numUnPackStreams,
        unPackSizes,
        digestsDefined,
        digests, allocMain->Alloc, allocTemp));
    db->ArchiveInfo.DataStartPosition += db->ArchiveInfo.StartPositionAfterHeader;
    RINOK(SzReadID(sd, &type));
  }

  if (type == k7zIdEnd)
    return SZ_OK;
  if (type != k7zIdFilesInfo)
    return SZE_ARCHIVE_ERROR;
  
  RINOK(SzReadNumber32(sd, &numFiles));
  db->Database.NumFiles = numFiles;

  MY_ALLOC(C7zFileItem, files, (size_t)numFiles, allocMain->Alloc);

  db->Database.Files = files;
  for(i = 0; i < numFiles; i++)
    SzFileInit(files + i);

  for (;;)
  {
    UInt64 type;
    UInt64 size;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    RINOK(SzReadNumber(sd, &size));

    if ((UInt64)(int)type != type)
    {
      RINOK(SzSkeepDataSize(sd, size));
    }
    else
    switch((int)type)
    {
      case k7zIdName:
      {
        RINOK(SzReadSwitch(sd));
        RINOK(SzReadFileNames(sd, numFiles, files, allocMain->Alloc))
        break;
      }
      case k7zIdEmptyStream:
      {
        RINOK(SzReadBoolVector(sd, numFiles, emptyStreamVector, allocTemp->Alloc));
        numEmptyStreams = 0;
        for (i = 0; i < numFiles; i++)
          if ((*emptyStreamVector)[i])
            numEmptyStreams++;
        break;
      }
      case k7zIdEmptyFile:
      {
        RINOK(SzReadBoolVector(sd, numEmptyStreams, emptyFileVector, allocTemp->Alloc));
        break;
      }
      case k7zIdLastWriteTime:
      {
        RINOK(SzReadBoolVector2(sd, numFiles, lwtVector, allocTemp->Alloc));
        RINOK(SzReadSwitch(sd));
        for (i = 0; i < numFiles; i++)
        {
          C7zFileItem *f = &files[i];
          Byte defined = (*lwtVector)[i];
          f->IsLastWriteTimeDefined = defined;
          f->LastWriteTime.Low = f->LastWriteTime.High = 0;
          if (defined)
          {
            RINOK(SzReadUInt32(sd, &f->LastWriteTime.Low));
            RINOK(SzReadUInt32(sd, &f->LastWriteTime.High));
          }
        }
        break;
      }
      default:
      {
        RINOK(SzSkeepDataSize(sd, size));
      }
    }
  }

  {
    UInt32 emptyFileIndex = 0;
    UInt32 sizeIndex = 0;
    for(i = 0; i < numFiles; i++)
    {
      C7zFileItem *file = files + i;
      file->IsAnti = 0;
      if (*emptyStreamVector == 0)
        file->HasStream = 1;
      else
        file->HasStream = (Byte)((*emptyStreamVector)[i] ? 0 : 1);
      if(file->HasStream)
      {
        file->IsDirectory = 0;
        file->Size = (*unPackSizes)[sizeIndex];
        file->FileCRC = (*digests)[sizeIndex];
        file->IsFileCRCDefined = (Byte)(*digestsDefined)[sizeIndex];
        sizeIndex++;
      }
      else
      {
        if (*emptyFileVector == 0)
          file->IsDirectory = 1;
        else
          file->IsDirectory = (Byte)((*emptyFileVector)[emptyFileIndex] ? 0 : 1);
        emptyFileIndex++;
        file->Size = 0;
        file->IsFileCRCDefined = 0;
      }
    }
  }
  return SzArDbExFill(db, allocMain->Alloc);
}

SZ_RESULT SzReadHeader(
    CSzData *sd, 
    CArchiveDatabaseEx *db, 
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  CFileSize *unPackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  Byte *emptyStreamVector = 0;
  Byte *emptyFileVector = 0;
  Byte *lwtVector = 0;
  SZ_RESULT res = SzReadHeader2(sd, db, 
      &unPackSizes, &digestsDefined, &digests,
      &emptyStreamVector, &emptyFileVector, &lwtVector, 
      allocMain, allocTemp);
  allocTemp->Free(unPackSizes);
  allocTemp->Free(digestsDefined);
  allocTemp->Free(digests);
  allocTemp->Free(emptyStreamVector);
  allocTemp->Free(emptyFileVector);
  allocTemp->Free(lwtVector);
  return res;
} 

SZ_RESULT SzReadAndDecodePackedStreams2(
    ISzInStream *inStream, 
    CSzData *sd,
    CSzByteBuffer *outBuffer,
    CFileSize baseOffset, 
    CArchiveDatabase *db,
    CFileSize **unPackSizes,
    Byte **digestsDefined,
    UInt32 **digests,
    #ifndef _LZMA_IN_CB
    Byte **inBuffer,
    #endif
    ISzAlloc *allocTemp)
{

  UInt32 numUnPackStreams = 0;
  CFileSize dataStartPos;
  CFolder *folder;
  #ifndef _LZMA_IN_CB
  CFileSize packSize = 0;
  UInt32 i = 0;
  #endif
  CFileSize unPackSize;
  SZ_RESULT res;

  RINOK(SzReadStreamsInfo(sd, &dataStartPos, db,
      &numUnPackStreams,  unPackSizes, digestsDefined, digests, 
      allocTemp->Alloc, allocTemp));
  
  dataStartPos += baseOffset;
  if (db->NumFolders != 1)
    return SZE_ARCHIVE_ERROR;

  folder = db->Folders;
  unPackSize = SzFolderGetUnPackSize(folder);
  
  RINOK(inStream->Seek(inStream, dataStartPos));

  #ifndef _LZMA_IN_CB
  for (i = 0; i < db->NumPackStreams; i++)
    packSize += db->PackSizes[i];

  MY_ALLOC(Byte, *inBuffer, (size_t)packSize, allocTemp->Alloc);

  RINOK(SafeReadDirect(inStream, *inBuffer, (size_t)packSize));
  #endif

  if (!SzByteBufferCreate(outBuffer, (size_t)unPackSize, allocTemp->Alloc))
    return SZE_OUTOFMEMORY;
  size_t nowPos = 0;
  res = SzDecode(db->PackSizes, folder, 
          #ifdef _LZMA_IN_CB
          inStream, dataStartPos, 
          #else
          *inBuffer, 
          #endif
          outBuffer->Items, (size_t)unPackSize, allocTemp, &nowPos, NULL, NULL, 0, NULL, NULL);
  RINOK(res)
  if (folder->UnPackCRCDefined)
    if (CrcCalc(outBuffer->Items, (size_t)unPackSize) != folder->UnPackCRC)
      return SZE_FAIL;
  return SZ_OK;
}

SZ_RESULT SzReadAndDecodePackedStreams(
    ISzInStream *inStream, 
    CSzData *sd,
    CSzByteBuffer *outBuffer,
    CFileSize baseOffset, 
    ISzAlloc *allocTemp)
{
  CArchiveDatabase db;
  CFileSize *unPackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  #ifndef _LZMA_IN_CB
  Byte *inBuffer = 0;
  #endif
  SZ_RESULT res;
  SzArchiveDatabaseInit(&db);
  res = SzReadAndDecodePackedStreams2(inStream, sd, outBuffer, baseOffset, 
    &db, &unPackSizes, &digestsDefined, &digests, 
    #ifndef _LZMA_IN_CB
    &inBuffer,
    #endif
    allocTemp);
  SzArchiveDatabaseFree(&db, allocTemp->Free);
  allocTemp->Free(unPackSizes);
  allocTemp->Free(digestsDefined);
  allocTemp->Free(digests);
  #ifndef _LZMA_IN_CB
  allocTemp->Free(inBuffer);
  #endif
  return res;
}

SZ_RESULT SzArchiveOpen2(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  Byte signature[k7zSignatureSize];
  Byte version;
  UInt32 crcFromArchive;
  UInt64 nextHeaderOffset;
  UInt64 nextHeaderSize;
  UInt32 nextHeaderCRC;
  UInt32 crc = 0;
  CFileSize pos = 0;
  CSzByteBuffer buffer;
  CSzData sd;
  SZ_RESULT res;

  RINOK(SafeReadDirect(inStream, signature, k7zSignatureSize));

  if (!TestSignatureCandidate(signature))
    return SZE_ARCHIVE_ERROR;

  /*
  db.Clear();
  db.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;
  */
  RINOK(SafeReadDirectByte(inStream, &version));
  if (version != k7zMajorVersion)
    return SZE_ARCHIVE_ERROR;
  RINOK(SafeReadDirectByte(inStream, &version));

  RINOK(SafeReadDirectUInt32(inStream, &crcFromArchive, &crc));

  crc = CRC_INIT_VAL;
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderOffset, &crc));
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderSize, &crc));
  RINOK(SafeReadDirectUInt32(inStream, &nextHeaderCRC, &crc));

  pos = k7zStartHeaderSize;
  db->ArchiveInfo.StartPositionAfterHeader = pos;
  
  if (CRC_GET_DIGEST(crc) != crcFromArchive)
    return SZE_ARCHIVE_ERROR;

  if (nextHeaderSize == 0)
    return SZ_OK;

  RINOK(inStream->Seek(inStream, (CFileSize)(pos + nextHeaderOffset)));

  if (!SzByteBufferCreate(&buffer, (size_t)nextHeaderSize, allocTemp->Alloc))
    return SZE_OUTOFMEMORY;

  res = SafeReadDirect(inStream, buffer.Items, (size_t)nextHeaderSize);
  if (res == SZ_OK)
  {
    res = SZE_ARCHIVE_ERROR;
    if (CrcCalc(buffer.Items, (UInt32)nextHeaderSize) == nextHeaderCRC)
    {
      for (;;)
      {
        UInt64 type;
        sd.Data = buffer.Items;
        sd.Size = buffer.Capacity;
        res = SzReadID(&sd, &type);
        if (res != SZ_OK)
          break;
        if (type == k7zIdHeader)
        {
          res = SzReadHeader(&sd, db, allocMain, allocTemp);
          break;
        }
        if (type != k7zIdEncodedHeader)
        {
          res = SZE_ARCHIVE_ERROR;
          break;
        }
        {
          CSzByteBuffer outBuffer;
          res = SzReadAndDecodePackedStreams(inStream, &sd, &outBuffer, 
              db->ArchiveInfo.StartPositionAfterHeader, 
              allocTemp);
          if (res != SZ_OK)
          {
            SzByteBufferFree(&outBuffer, allocTemp->Free);
            break;
          }
          SzByteBufferFree(&buffer, allocTemp->Free);
          buffer.Items = outBuffer.Items;
          buffer.Capacity = outBuffer.Capacity;
        }
      }
    }
  }
  SzByteBufferFree(&buffer, allocTemp->Free);
  return res;
}

SZ_RESULT SzArchiveOpen(
    ISzInStream *inStream, 
    CArchiveDatabaseEx *db,
    ISzAlloc *allocMain, 
    ISzAlloc *allocTemp)
{
  SZ_RESULT res = SzArchiveOpen2(inStream, db, allocMain, allocTemp);
  if (res != SZ_OK)
    SzArDbExFree(db, allocMain->Free);
  return res;
}
