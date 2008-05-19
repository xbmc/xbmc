/* 
LzmaTest.c
Test application for LZMA Decoder

This file written and distributed to public domain by Igor Pavlov.
This file is part of LZMA SDK 4.26 (2005-08-05)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LzmaDecode.h"

const char *kCantReadMessage = "Can not read input file";
const char *kCantWriteMessage = "Can not write output file";
const char *kCantAllocateMessage = "Can not allocate memory";

size_t MyReadFile(FILE *file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  return fread(data, 1, size, file); 
}

int MyReadFileAndCheck(FILE *file, void *data, size_t size)
  { return (MyReadFile(file, data, size) == size);} 

size_t MyWriteFile(FILE *file, const void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  return fwrite(data, 1, size, file); 
}

int MyWriteFileAndCheck(FILE *file, const void *data, size_t size)
  { return (MyWriteFile(file, data, size) == size); }

#ifdef _LZMA_IN_CB
#define kInBufferSize (1 << 15)
typedef struct _CBuffer
{
  ILzmaInCallback InCallback;
  FILE *File;
  unsigned char Buffer[kInBufferSize];
} CBuffer;

int LzmaReadCompressed(void *object, const unsigned char **buffer, SizeT *size)
{
  CBuffer *b = (CBuffer *)object;
  *buffer = b->Buffer;
  *size = (SizeT)MyReadFile(b->File, b->Buffer, kInBufferSize);
  return LZMA_RESULT_OK;
}
CBuffer g_InBuffer;

#endif

#ifdef _LZMA_OUT_READ
#define kOutBufferSize (1 << 15)
unsigned char g_OutBuffer[kOutBufferSize];
#endif

int PrintError(char *buffer, const char *message)
{
  sprintf(buffer + strlen(buffer), "\nError: ");
  sprintf(buffer + strlen(buffer), message);
  return 1;
}

int main3(FILE *inFile, FILE *outFile, char *rs)
{
  /* We use two 32-bit integers to construct 64-bit integer for file size.
     You can remove outSizeHigh, if you don't need >= 4GB supporting,
     or you can use UInt64 outSize, if your compiler supports 64-bit integers*/
  UInt32 outSize = 0;
  UInt32 outSizeHigh = 0;
  #ifndef _LZMA_OUT_READ
  SizeT outSizeFull;
  unsigned char *outStream;
  #endif
  
  int waitEOS = 1; 
  /* waitEOS = 1, if there is no uncompressed size in headers, 
   so decoder will wait EOS (End of Stream Marker) in compressed stream */

  #ifndef _LZMA_IN_CB
  SizeT compressedSize;
  unsigned char *inStream;
  #endif

  CLzmaDecoderState state;  /* it's about 24-80 bytes structure, if int is 32-bit */
  unsigned char properties[LZMA_PROPERTIES_SIZE];

  int res;

  #ifdef _LZMA_IN_CB
  g_InBuffer.File = inFile;
  #endif

  if (sizeof(UInt32) < 4)
    return PrintError(rs, "LZMA decoder needs correct UInt32");

  #ifndef _LZMA_IN_CB
  {
    long length;
    fseek(inFile, 0, SEEK_END);
    length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);
    if ((long)(SizeT)length != length)
      return PrintError(rs, "Too big compressed stream");
    compressedSize = (SizeT)(length - (LZMA_PROPERTIES_SIZE + 8));
  }
  #endif

  /* Read LZMA properties for compressed stream */

  if (!MyReadFileAndCheck(inFile, properties, sizeof(properties)))
    return PrintError(rs, kCantReadMessage);

  /* Read uncompressed size */

  {
    int i;
    for (i = 0; i < 8; i++)
    {
      unsigned char b;
      if (!MyReadFileAndCheck(inFile, &b, 1))
        return PrintError(rs, kCantReadMessage);
      if (b != 0xFF)
        waitEOS = 0;
      if (i < 4)
        outSize += (UInt32)(b) << (i * 8);
      else
        outSizeHigh += (UInt32)(b) << ((i - 4) * 8);
    }
    
    #ifndef _LZMA_OUT_READ
    if (waitEOS)
      return PrintError(rs, "Stream with EOS marker is not supported");
    outSizeFull = (SizeT)outSize;
    if (sizeof(SizeT) >= 8)
      outSizeFull |= (((SizeT)outSizeHigh << 16) << 16);
    else if (outSizeHigh != 0 || (UInt32)(SizeT)outSize != outSize)
      return PrintError(rs, "Too big uncompressed stream");
    #endif
  }

  /* Decode LZMA properties and allocate memory */
  
  if (LzmaDecodeProperties(&state.Properties, properties, LZMA_PROPERTIES_SIZE) != LZMA_RESULT_OK)
    return PrintError(rs, "Incorrect stream properties");
  state.Probs = (CProb *)malloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));

  #ifdef _LZMA_OUT_READ
  if (state.Properties.DictionarySize == 0)
    state.Dictionary = 0;
  else
    state.Dictionary = (unsigned char *)malloc(state.Properties.DictionarySize);
  #else
  if (outSizeFull == 0)
    outStream = 0;
  else
    outStream = (unsigned char *)malloc(outSizeFull);
  #endif

  #ifndef _LZMA_IN_CB
  if (compressedSize == 0)
    inStream = 0;
  else
    inStream = (unsigned char *)malloc(compressedSize);
  #endif

  if (state.Probs == 0 
    #ifdef _LZMA_OUT_READ
    || (state.Dictionary == 0 && state.Properties.DictionarySize != 0)
    #else
    || (outStream == 0 && outSizeFull != 0)
    #endif
    #ifndef _LZMA_IN_CB
    || (inStream == 0 && compressedSize != 0)
    #endif
    )
  {
    free(state.Probs);
    #ifdef _LZMA_OUT_READ
    free(state.Dictionary);
    #else
    free(outStream);
    #endif
    #ifndef _LZMA_IN_CB
    free(inStream);
    #endif
    return PrintError(rs, kCantAllocateMessage);
  }

  /* Decompress */

  #ifdef _LZMA_IN_CB
  g_InBuffer.InCallback.Read = LzmaReadCompressed;
  #else
  if (!MyReadFileAndCheck(inFile, inStream, compressedSize))
    return PrintError(rs, kCantReadMessage);
  #endif

  #ifdef _LZMA_OUT_READ
  {
    #ifndef _LZMA_IN_CB
    SizeT inAvail = compressedSize;
    const unsigned char *inBuffer = inStream;
    #endif
    LzmaDecoderInit(&state);
    do
    {
      #ifndef _LZMA_IN_CB
      SizeT inProcessed;
      #endif
      SizeT outProcessed;
      SizeT outAvail = kOutBufferSize;
      if (!waitEOS && outSizeHigh == 0 && outAvail > outSize)
        outAvail = (SizeT)outSize;
      res = LzmaDecode(&state,
        #ifdef _LZMA_IN_CB
        &g_InBuffer.InCallback,
        #else
        inBuffer, inAvail, &inProcessed,
        #endif
        g_OutBuffer, outAvail, &outProcessed);
      if (res != 0)
      {
        sprintf(rs + strlen(rs), "\nDecoding error = %d\n", res);
        res = 1;
        break;
      }
      #ifndef _LZMA_IN_CB
      inAvail -= inProcessed;
      inBuffer += inProcessed;
      #endif
      
      if (outFile != 0)  
        if (!MyWriteFileAndCheck(outFile, g_OutBuffer, (size_t)outProcessed))
        {
          PrintError(rs, kCantWriteMessage);
          res = 1;
          break;
        }
        
      if (outSize < outProcessed)
        outSizeHigh--;
      outSize -= (UInt32)outProcessed;
      outSize &= 0xFFFFFFFF;
        
      if (outProcessed == 0)
      {
        if (!waitEOS && (outSize != 0 || outSizeHigh != 0))
          res = 1;
        break;
      }
    }
    while ((outSize != 0 && outSizeHigh == 0) || outSizeHigh != 0  || waitEOS);
  }

  #else
  {
    #ifndef _LZMA_IN_CB
    SizeT inProcessed;
    #endif
    SizeT outProcessed;
    res = LzmaDecode(&state,
      #ifdef _LZMA_IN_CB
      &g_InBuffer.InCallback,
      #else
      inStream, compressedSize, &inProcessed,
      #endif
      outStream, outSizeFull, &outProcessed);
    if (res != 0)
    {
      sprintf(rs + strlen(rs), "\nDecoding error = %d\n", res);
      res = 1;
    }
    else if (outFile != 0)
    {
      if (!MyWriteFileAndCheck(outFile, outStream, (size_t)outProcessed))
      {
        PrintError(rs, kCantWriteMessage);
        res = 1;
      }
    }
  }
  #endif

  free(state.Probs);
  #ifdef _LZMA_OUT_READ
  free(state.Dictionary);
  #else
  free(outStream);
  #endif
  #ifndef _LZMA_IN_CB
  free(inStream);
  #endif
  return res;
}

int main2(int numArgs, const char *args[], char *rs)
{
  FILE *inFile = 0;
  FILE *outFile = 0;
  int res;

  sprintf(rs + strlen(rs), "\nLZMA Decoder 4.26 Copyright (c) 1999-2005 Igor Pavlov  2005-08-05\n");
  if (numArgs < 2 || numArgs > 3)
  {
    sprintf(rs + strlen(rs), "\nUsage:  lzmadec file.lzma [outFile]\n");
    return 1;
  }

  inFile = fopen(args[1], "rb");
  if (inFile == 0)
    return PrintError(rs, "Can not open input file");

  if (numArgs > 2)
  {
    outFile = fopen(args[2], "wb+");
    if (outFile == 0)
      return PrintError(rs, "Can not open output file");
  }

  res = main3(inFile, outFile, rs);

  if (outFile != 0)
    fclose(outFile);
  fclose(inFile);
  return res;
}

int main(int numArgs, const char *args[])
{
  char rs[800] = { 0 };
  int res = main2(numArgs, args, rs);
  printf(rs);
  return res;
}
