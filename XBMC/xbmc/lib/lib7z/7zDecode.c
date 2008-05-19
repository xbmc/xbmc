/* 7zDecode.c */

#include <memory.h>

#include "7zDecode.h"
#ifdef _SZ_ONE_DIRECTORY
#include "LzmaDecode.h"
#else
#include "LzmaDecode.h"
#include "BranchX86.h"
#include "BranchX86_2.h"
#endif

#define k_Copy 0
#define k_LZMA 0x30101
#define k_BCJ 0x03030103
#define k_BCJ2 0x0303011B

#ifdef _LZMA_IN_CB

typedef struct _CLzmaInCallbackImp
{
  ILzmaInCallback InCallback;
  ISzInStream *InStream;
  CFileSize Size;
} CLzmaInCallbackImp;

int LzmaReadImp(void *object, const unsigned char **buffer, SizeT *size)
{
  CLzmaInCallbackImp *cb = (CLzmaInCallbackImp *)object;
  size_t processedSize;
  SZ_RESULT res;
  size_t curSize = (1 << 20);
  if (curSize > cb->Size)
    curSize = (size_t)cb->Size;
  *size = 0;
  res = cb->InStream->Read((void *)cb->InStream, (void **)buffer, curSize, &processedSize);
  *size = (SizeT)processedSize;
  if (processedSize > curSize)
    return (int)SZE_FAIL;
  cb->Size -= processedSize;
  if (res == SZ_OK)
    return 0;
  return (int)res;
}

#endif

SZ_RESULT SzDecodeLzma(CCoderInfo *coder, CFileSize inSize,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain, size_t *nowPos, WriteCache *writeCache, void *writeOBJECT, int SizeToCache, ReadCache *readCache, void *readOBJECT)
{
  #ifdef _LZMA_IN_CB
  CLzmaInCallbackImp lzmaCallback;
  #else
  SizeT inProcessed;
  #endif
  
  CLzmaDecoderState state;  /* it's about 24-80 bytes structure, if int is 32-bit */
  int result;
  SizeT outSizeProcessedLoc;
  
  #ifdef _LZMA_IN_CB
  lzmaCallback.Size = inSize;
  lzmaCallback.InStream = inStream;
  lzmaCallback.InCallback.Read = LzmaReadImp;
  #endif
  
  if (LzmaDecodeProperties(&state.Properties, coder->Properties.Items, 
      (unsigned)coder->Properties.Capacity) != LZMA_RESULT_OK)
    return SZE_FAIL;
  
  state.Probs = (CProb *)allocMain->Alloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
  if (state.Probs == 0)
    return SZE_OUTOFMEMORY;
  
  #ifdef _LZMA_OUT_READ
  if (state.Properties.DictionarySize == 0)
    state.Dictionary = 0;
  else
  {
    state.Dictionary = (unsigned char *)allocMain->Alloc(state.Properties.DictionarySize);
    if (state.Dictionary == 0)
    {
      allocMain->Free(state.Probs);
      return SZE_OUTOFMEMORY;
    }
  }
  LzmaDecoderInit(&state);
  #endif

  result = LzmaDecode(&state,
  #ifdef _LZMA_IN_CB
    &lzmaCallback.InCallback,
  #else
    inBuffer, (SizeT)inSize, &inProcessed,
  #endif
    outBuffer, (SizeT)outSize, &outSizeProcessedLoc, nowPos, writeCache, writeOBJECT, SizeToCache, readCache, readOBJECT);
  allocMain->Free(state.Probs);
  #ifdef _LZMA_OUT_READ
  allocMain->Free(state.Dictionary);
  #endif
  if (result == LZMA_RESULT_DATA_ERROR)
    return SZE_DATA_ERROR;
  if (result != LZMA_RESULT_OK)
    return SZE_FAIL;
  return (outSizeProcessedLoc == outSize) ? SZ_OK : SZE_DATA_ERROR;
}

#ifdef _LZMA_IN_CB
SZ_RESULT SzDecodeCopy(CFileSize inSize, ISzInStream *inStream, Byte *outBuffer)
{
  while (inSize > 0)
  {
    void *inBuffer;
    size_t processedSize, curSize = (1 << 18);
    if (curSize > inSize)
      curSize = (size_t)(inSize);
    RINOK(inStream->Read((void *)inStream, (void **)&inBuffer, curSize, &processedSize));
    if (processedSize == 0)
      return SZE_DATA_ERROR;
    if (processedSize > curSize)
      return SZE_FAIL;

    memcpy(outBuffer, inBuffer, processedSize);
    outBuffer += processedSize;
    inSize -= processedSize;
  }
  return SZ_OK;
}
#endif

#define IS_UNSUPPORTED_METHOD(m) ((m) != k_Copy && (m) != k_LZMA)
#define IS_UNSUPPORTED_CODER(c) (IS_UNSUPPORTED_METHOD(c.MethodID) || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ(c) (c.MethodID != k_BCJ || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ2(c) (c.MethodID != k_BCJ2 || c.NumInStreams != 4 || c.NumOutStreams != 1)

SZ_RESULT CheckSupportedFolder(const CFolder *f)
{
  if (f->NumCoders < 1 || f->NumCoders > 4)
    return SZE_NOTIMPL;
  if (IS_UNSUPPORTED_CODER(f->Coders[0]))
    return SZE_NOTIMPL;
  if (f->NumCoders == 1)
  {
    if (f->NumPackStreams != 1 || f->PackStreams[0] != 0 || f->NumBindPairs != 0)
      return SZE_NOTIMPL;
    return SZ_OK;
  }
  if (f->NumCoders == 2)
  {
    if (IS_NO_BCJ(f->Coders[1]) ||
        f->NumPackStreams != 1 || f->PackStreams[0] != 0 ||
        f->NumBindPairs != 1 ||
        f->BindPairs[0].InIndex != 1 || f->BindPairs[0].OutIndex != 0)
      return SZE_NOTIMPL;
    return SZ_OK;
  }
  if (f->NumCoders == 4)
  {
    if (IS_UNSUPPORTED_CODER(f->Coders[1]) ||
        IS_UNSUPPORTED_CODER(f->Coders[2]) ||
        IS_NO_BCJ2(f->Coders[3]))
      return SZE_NOTIMPL;
    if (f->NumPackStreams != 4 || 
        f->PackStreams[0] != 2 ||
        f->PackStreams[1] != 6 ||
        f->PackStreams[2] != 1 ||
        f->PackStreams[3] != 0 ||
        f->NumBindPairs != 3 ||
        f->BindPairs[0].InIndex != 5 || f->BindPairs[0].OutIndex != 0 ||
        f->BindPairs[1].InIndex != 4 || f->BindPairs[1].OutIndex != 1 ||
        f->BindPairs[2].InIndex != 3 || f->BindPairs[2].OutIndex != 2)
      return SZE_NOTIMPL;
    return SZ_OK;
  }
  return SZE_NOTIMPL;
}

CFileSize GetSum(const CFileSize *values, UInt32 index)
{
  CFileSize sum = 0;
  UInt32 i;
  for (i = 0; i < index; i++)
    sum += values[i];
  return sum;
}

SZ_RESULT SzDecode2(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream, CFileSize startPos,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain,
    Byte *tempBuf[], size_t *nowPos, WriteCache *writeCache, void *writeOBJECT, int SizeToCache, ReadCache *readCache, void *readOBJECT)
{
  UInt32 ci;
  size_t tempSizes[3] = { 0, 0, 0};
  size_t tempSize3 = 0;
  Byte *tempBuf3 = 0;

  RINOK(CheckSupportedFolder(folder));

  for (ci = 0; ci < folder->NumCoders; ci++)
  {
    CCoderInfo *coder = &folder->Coders[ci];

    if (coder->MethodID == k_Copy || coder->MethodID == k_LZMA)
    {
      UInt32 si = 0;
      CFileSize offset;
      CFileSize inSize;
      Byte *outBufCur = outBuffer;
      size_t outSizeCur = outSize;
      if (folder->NumCoders == 4)
      {
        UInt32 indices[] = { 3, 2, 0 };
        CFileSize unpackSize = folder->UnPackSizes[ci];
        si = indices[ci];
        if (ci < 2)
        {
          Byte *temp;
          outSizeCur = (size_t)unpackSize;
          if (outSizeCur != unpackSize)
            return SZE_OUTOFMEMORY;
          temp = (Byte *)allocMain->Alloc(outSizeCur);
          if (temp == 0 && outSizeCur != 0)
            return SZE_OUTOFMEMORY;
          outBufCur = tempBuf[1 - ci] = temp;
          tempSizes[1 - ci] = outSizeCur;
        }
        else if (ci == 2)
        {
          if (unpackSize > outSize)
            return SZE_OUTOFMEMORY;
          tempBuf3 = outBufCur = outBuffer + (outSize - (size_t)unpackSize);
          tempSize3 = outSizeCur = (size_t)unpackSize;
        }
        else
          return SZE_NOTIMPL;
      }
      offset = GetSum(packSizes, si);
      inSize = packSizes[si];
      #ifdef _LZMA_IN_CB
      RINOK(inStream->Seek(inStream, startPos + offset));
      #endif

      if (coder->MethodID == k_Copy)
      {
        if (inSize != outSizeCur)
          return SZE_DATA_ERROR;
        #ifdef _LZMA_IN_CB
        RINOK(SzDecodeCopy(inSize, inStream, outBufCur));
        #else
        memcpy(outBufCur, inBuffer + (size_t)offset, (size_t)inSize);
        #endif
      }
      else
      {
        SZ_RESULT res = SzDecodeLzma(coder, inSize,
            #ifdef _LZMA_IN_CB
            inStream,
            #else
            inBuffer + (size_t)offset,
            #endif
            outBufCur, outSizeCur, allocMain, nowPos, writeCache, writeOBJECT, SizeToCache, readCache, readOBJECT);
        RINOK(res)
      }
    }
    else if (coder->MethodID == k_BCJ)
    {
      UInt32 state;
      if (ci != 1)
        return SZE_NOTIMPL;
      x86_Convert_Init(state);
      x86_Convert(outBuffer, outSize, 0, &state, 0);
    }
    else if (coder->MethodID == k_BCJ2)
    {
      CFileSize offset = GetSum(packSizes, 1);
      CFileSize s3Size = packSizes[1];
      SZ_RESULT res;
      if (ci != 3)
        return SZE_NOTIMPL;

      #ifdef _LZMA_IN_CB
      RINOK(inStream->Seek(inStream, startPos + offset));
      tempSizes[2] = (size_t)s3Size;
      if (tempSizes[2] != s3Size)
        return SZE_OUTOFMEMORY;
      tempBuf[2] = (Byte *)allocMain->Alloc(tempSizes[2]);
      if (tempBuf[2] == 0 && tempSizes[2] != 0)
        return SZE_OUTOFMEMORY;
      res = SzDecodeCopy(s3Size, inStream, tempBuf[2]);
      RINOK(res)
      #endif

      res = x86_2_Decode(
          tempBuf3, tempSize3, 
          tempBuf[0], tempSizes[0], 
          tempBuf[1], tempSizes[1], 
          #ifdef _LZMA_IN_CB
          tempBuf[2], tempSizes[2], 
          #else
          inBuffer + (size_t)offset, (size_t)s3Size, 
          #endif
          outBuffer, outSize);
      RINOK(res)
    }
    else 
      return SZE_NOTIMPL;
  }
  return SZ_OK;
}

SZ_RESULT SzDecode(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream, CFileSize startPos,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain, size_t *nowPos, WriteCache *writeCache, void *writeOBJECT, int SizeToCache, ReadCache *readCache, void *readOBJECT)
{
  Byte *tempBuf[3] = { 0, 0, 0};
  int i;
  SZ_RESULT res = SzDecode2(packSizes, folder,
      #ifdef _LZMA_IN_CB
      inStream, startPos,
      #else
      inBuffer,
      #endif
      outBuffer, outSize, allocMain, tempBuf, nowPos, writeCache, writeOBJECT, SizeToCache, readCache, readOBJECT);
  for (i = 0; i < 3; i++)
    allocMain->Free(tempBuf[i]);
  return res;
}
